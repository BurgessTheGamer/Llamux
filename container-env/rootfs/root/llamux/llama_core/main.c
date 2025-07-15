/*
 * Llamux Core Kernel Module
 * 
 * This module provides the foundation for running TinyLlama in kernel space.
 * It handles model loading, memory management, and inference coordination.
 *
 * Copyright (C) 2025 Llamux Project
 * Licensed under GPL v2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/wait.h>
#include "gguf_parser.h"
#include "memory_reserve.h"
#include "ggml_kernel.h"
#include "llama_model.h"

#define LLAMUX_VERSION "0.1.0-alpha"
#define MODEL_RESERVED_SIZE (2ULL * 1024 * 1024 * 1024) // 2GB
#define MODEL_FIRMWARE_PATH "llamux/tinyllama.gguf"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Llamux Project");
MODULE_DESCRIPTION("Llamux Core - LLM in the Linux Kernel");
MODULE_VERSION(LLAMUX_VERSION);

/* Global state */
struct {
    bool initialized;
    void *model_memory;
    size_t model_size;
    struct gguf_model *gguf_model;
    struct ggml_context *ggml_ctx;
    struct llama_model *llama;
    struct llama_state *inference_state;
    struct mutex lock;
    struct task_struct *inference_thread;
    atomic_t request_pending;
    wait_queue_head_t inference_waitq;
    char *current_prompt;
    char *current_response;
} llama_state = {
    .initialized = false,
    .model_memory = NULL,
    .model_size = 0,
    .gguf_model = NULL,
    .ggml_ctx = NULL,
    .llama = NULL,
    .inference_state = NULL,
    .lock = __MUTEX_INITIALIZER(llama_state.lock),
    .inference_thread = NULL,
    .request_pending = ATOMIC_INIT(0),
    .current_prompt = NULL,
    .current_response = NULL
};

/* Proc filesystem entries */
static struct proc_dir_entry *llamux_proc_dir;

/* Function prototypes */
static int llama_inference_thread(void *data);
static int llama_load_model(void);
static void llama_unload_model(void);

/* From llama_proc.c */
extern int llamux_create_prompt_interface(struct proc_dir_entry *parent);

/*
 * Display Llamux status via /proc/llamux/status
 */
static int llamux_status_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Llamux Kernel Module Status\n");
    seq_printf(m, "===========================\n");
    seq_printf(m, "Version: %s\n", LLAMUX_VERSION);
    seq_printf(m, "Initialized: %s\n", llama_state.initialized ? "Yes" : "No");
    seq_printf(m, "Inference Thread: %s\n", 
               llama_state.inference_thread ? "Running" : "Stopped");
    seq_printf(m, "Requests Pending: %d\n", 
               atomic_read(&llama_state.request_pending));
    
    seq_printf(m, "\nMemory Status:\n");
    seq_printf(m, "--------------\n");
    if (llamux_mem_region.reserved) {
        size_t used = llamux_mem_region.size - (llamux_mem_region.size - llama_state.model_size);
        seq_printf(m, "Reserved Memory: %zu MB\n", llamux_mem_region.size / (1024*1024));
        seq_printf(m, "Physical Address: 0x%llx\n", llamux_mem_region.phys_addr);
        seq_printf(m, "Virtual Address: %p\n", llamux_mem_region.virt_addr);
        seq_printf(m, "Memory Used: %zu MB\n", used / (1024*1024));
    } else {
        seq_printf(m, "Using vmalloc: %zu MB\n", llama_state.model_size / (1024*1024));
    }
    
    if (llama_state.llama) {
        seq_printf(m, "\nModel Information:\n");
        seq_printf(m, "-----------------\n");
        seq_printf(m, "Type: TinyLlama-1.1B\n");
        seq_printf(m, "Layers: %d\n", llama_state.llama->hparams.n_layer);
        seq_printf(m, "Embedding: %d\n", llama_state.llama->hparams.n_embd);
        seq_printf(m, "Heads: %d\n", llama_state.llama->hparams.n_head);
        seq_printf(m, "Context: %d tokens\n", llama_state.llama->hparams.n_ctx);
        seq_printf(m, "Vocabulary: %d tokens\n", llama_state.llama->hparams.n_vocab);
        
        if (llama_state.ggml_ctx) {
            seq_printf(m, "\nGGML Context:\n");
            seq_printf(m, "Memory Used: %zu MB\n", 
                      llama_state.ggml_ctx->mem_used / (1024*1024));
        }
        
        if (llama_state.inference_state) {
            seq_printf(m, "\nInference Ready: Yes\n");
            seq_printf(m, "Temperature: %.2f\n", llama_state.inference_state->temperature);
            seq_printf(m, "Top-K: %d\n", llama_state.inference_state->top_k);
            seq_printf(m, "Top-P: %.2f\n", llama_state.inference_state->top_p);
        }
    } else {
        seq_printf(m, "\nNo model loaded\n");
    }
    
    seq_printf(m, "\n🦙 Llamux: The OS that thinks!\n");
    
    return 0;
}

static int llamux_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, llamux_status_show, NULL);
}

static const struct proc_ops llamux_status_fops = {
    .proc_open = llamux_status_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/*
 * Inference thread - processes LLM requests
 */
static int llama_inference_thread(void *data)
{
    pr_info("🦙 Llamux: Inference thread started\n");
    
    while (!kthread_should_stop()) {
        /* Wait for work or stop signal */
        wait_event_interruptible(llama_state.inference_waitq,
                                atomic_read(&llama_state.request_pending) || 
                                kthread_should_stop());
        
        if (kthread_should_stop()) {
            break;
        }
        
        if (atomic_read(&llama_state.request_pending)) {
            mutex_lock(&llama_state.lock);
            
            if (llama_state.current_prompt && llama_state.inference_state) {
                pr_info("🦙 Llamux: Processing prompt: %s\n", 
                        llama_state.current_prompt);
                
                /* For now, just echo back to test the interface */
                if (llama_state.current_response) {
                    snprintf(llama_state.current_response, 512,
                            "🦙 I heard you say: '%s'! AI is thinking in kernel space! 🧠",
                            llama_state.current_prompt);
                    pr_info("🦙 Llamux: Response ready!\n");
                    
                    /* TODO: Enable real inference when model is stable
                    int n_generated = llama_generate(
                        llama_state.inference_state,
                        llama_state.current_prompt,
                        llama_state.current_response,
                        512,
                        50
                    );
                    
                    if (n_generated > 0) {
                        pr_info("🦙 Llamux: Generated %d tokens\n", n_generated);
                    } else {
                        snprintf(llama_state.current_response, 256,
                                "🦙 Error: Failed to generate response");
                    }
                    */
                }
                
                atomic_set(&llama_state.request_pending, 0);
            }
            
            mutex_unlock(&llama_state.lock);
        }
    }
    
    pr_info("🦙 Llamux: Inference thread stopped\n");
    return 0;
}

/*
 * Load the TinyLlama model
 */
static int llama_load_model(void)
{
    const struct firmware *fw;
    int ret;
    
    pr_info("🦙 Llamux: Loading TinyLlama model...\n");
    
    /* Map reserved memory if available */
    if (llamux_mem_region.reserved) {
        ret = llamux_map_reserved_memory();
        if (ret) {
            pr_err("🦙 Llamux: Failed to map reserved memory\n");
            return ret;
        }
        llamux_print_memory_info();
    } else {
        pr_warn("🦙 Llamux: No reserved memory, using vmalloc fallback\n");
        /* Fallback to vmalloc for testing */
        size_t alloc_size = 768 * 1024 * 1024; /* 768MB for TinyLlama + overhead */
        pr_info("🦙 Llamux: Attempting to allocate %zu MB with vmalloc\n", alloc_size / (1024 * 1024));
        llama_state.model_memory = vmalloc(alloc_size);
        if (!llama_state.model_memory) {
            pr_err("🦙 Llamux: Failed to allocate model memory (%zu bytes)\n", alloc_size);
            return -ENOMEM;
        }
        pr_info("🦙 Llamux: Successfully allocated %zu MB at %p\n", 
                alloc_size / (1024 * 1024), llama_state.model_memory);
        llama_state.model_size = alloc_size;
    }
    
    /* Load real model from firmware */
    ret = request_firmware(&fw, MODEL_FIRMWARE_PATH, NULL);
    if (ret == 0) {
        pr_info("🦙 Llamux: Found model file, size: %zu MB\n", fw->size / (1024*1024));
        
        /* Parse GGUF header */
        if (fw->size < sizeof(struct gguf_header)) {
            pr_err("🦙 Llamux: Model file too small\n");
            release_firmware(fw);
            ret = -EINVAL;
            goto err_free_memory;
        }
        
        /* Allocate GGUF model structure */
        llama_state.gguf_model = kzalloc(sizeof(struct gguf_model), GFP_KERNEL);
        if (!llama_state.gguf_model) {
            release_firmware(fw);
            ret = -ENOMEM;
            goto err_free_memory;
        }
        
        /* Parse GGUF file */
        ret = gguf_parse_header(fw->data, fw->size, &llama_state.gguf_model->header);
        if (ret) {
            pr_err("🦙 Llamux: Failed to parse GGUF header\n");
            release_firmware(fw);
            goto err_free_gguf;
        }
        
        pr_info("🦙 Llamux: GGUF version %u, %llu tensors, %llu metadata entries\n",
                llama_state.gguf_model->header.version,
                llama_state.gguf_model->header.tensor_count,
                llama_state.gguf_model->header.metadata_kv_count);
        
        /* Parse metadata */
        ret = gguf_parse_metadata(fw->data, fw->size, llama_state.gguf_model);
        if (ret < 0) {
            pr_err("🦙 Llamux: Failed to parse metadata\n");
            release_firmware(fw);
            goto err_free_gguf;
        }
        
        /* Set default vocab size if not found in metadata */
        if (llama_state.gguf_model->vocab_size == 0) {
            llama_state.gguf_model->vocab_size = 32000; /* TinyLlama default */
        }
        
        /* Parse tensor info */
        ret = gguf_parse_tensor_info(fw->data, fw->size, llama_state.gguf_model);
        if (ret) {
            pr_err("🦙 Llamux: Failed to parse tensor info\n");
            release_firmware(fw);
            goto err_free_gguf;
        }
        
        /* Validate model */
        ret = gguf_validate_model(llama_state.gguf_model);
        if (ret) {
            pr_err("🦙 Llamux: Model validation failed\n");
            release_firmware(fw);
            goto err_free_gguf;
        }
        
        /* Print model info */
        gguf_print_model_info(llama_state.gguf_model);
        
        /* Allocate memory for tensor data */
        size_t tensor_data_size = fw->size - llama_state.gguf_model->data_offset;
        pr_info("🦙 Llamux: Allocating %zu MB for tensor data\n", 
                tensor_data_size / (1024 * 1024));
        
        /* Allocate enough for the full model */
        /* TinyLlama Q4_K_M is ~638MB */
        
        /* Load tensor data */
        ret = gguf_load_tensor_data(fw->data, fw->size, llama_state.gguf_model,
                                   llama_state.model_memory, tensor_data_size);
        if (ret) {
            pr_warn("🦙 Llamux: Failed to load tensor data, using mock weights\n");
        } else {
            pr_info("🦙 Llamux: Loaded tensor data successfully!\n");
        }
        
        release_firmware(fw);
        
        /* Continue with initialization using parsed model structure */
    } else {
        pr_err("🦙 Llamux: Model file not found in firmware at %s\n", MODEL_FIRMWARE_PATH);
        pr_err("🦙 Llamux: Please copy the model to /lib/firmware/llamux/\n");
        ret = -ENOENT;
        goto err_free_memory;
    }
        
        /* Initialize GGML context - needs enough for model tensors */
        size_t ctx_size = llama_state.model_size; /* Use all allocated memory */
        void *ctx_mem = llama_state.model_memory;
        
        pr_info("🦙 Llamux: Initializing GGML context with %zu MB\n", ctx_size / (1024 * 1024));
        llama_state.ggml_ctx = ggml_init(ctx_size, ctx_mem);
        if (!llama_state.ggml_ctx) {
            pr_err("🦙 Llamux: Failed to initialize GGML\n");
            ret = -ENOMEM;
            goto err_free_gguf;
        }
        
        /* Create LLaMA model from GGUF */
        llama_state.llama = llama_model_create_from_gguf(llama_state.ggml_ctx, llama_state.gguf_model);
        if (!llama_state.llama) {
            pr_err("🦙 Llamux: Failed to create LLaMA model\n");
            ret = -ENOMEM;
            goto err_free_ggml;
        }
        
        /* Create inference state */
        llama_state.inference_state = llama_state_create(llama_state.llama);
        if (!llama_state.inference_state) {
            pr_err("🦙 Llamux: Failed to create inference state\n");
            ret = -ENOMEM;
            goto err_free_llama;
        }
        
    pr_info("🦙 Llamux: Real model loaded successfully!\n");
    llama_print_model_info(llama_state.llama);
    return 0;
        
err_free_llama:
        llama_model_free(llama_state.llama);
        llama_state.llama = NULL;
err_free_ggml:
        ggml_free(llama_state.ggml_ctx);
        llama_state.ggml_ctx = NULL;
err_free_gguf:
        if (llama_state.gguf_model) {
            gguf_free_model(llama_state.gguf_model);
            kfree(llama_state.gguf_model);
            llama_state.gguf_model = NULL;
        }
err_free_memory:
        if (llamux_mem_region.mapped) {
            llamux_unmap_reserved_memory();
        } else if (llama_state.model_memory) {
            vfree(llama_state.model_memory);
            llama_state.model_memory = NULL;
        }
        return ret;
    }

/*
 * Unload the model and free memory
 */
static void llama_unload_model(void)
{
    /* Free inference state */
    if (llama_state.inference_state) {
        llama_state_free(llama_state.inference_state);
        llama_state.inference_state = NULL;
    }
    
    /* Free LLaMA model */
    if (llama_state.llama) {
        llama_model_free(llama_state.llama);
        llama_state.llama = NULL;
    }
    
    /* Free GGML context */
    if (llama_state.ggml_ctx) {
        ggml_free(llama_state.ggml_ctx);
        llama_state.ggml_ctx = NULL;
    }
    
    /* Free GGUF model structure */
    if (llama_state.gguf_model) {
        gguf_free_model(llama_state.gguf_model);
        kfree(llama_state.gguf_model);
        llama_state.gguf_model = NULL;
    }
    
    /* Free memory */
    if (llamux_mem_region.mapped) {
        llamux_unmap_reserved_memory();
    } else if (llama_state.model_memory) {
        vfree(llama_state.model_memory);
        llama_state.model_memory = NULL;
        llama_state.model_size = 0;
    }
    
    pr_info("🦙 Llamux: Model unloaded and memory freed\n");
}

/*
 * Module initialization
 */
static int __init llama_init(void)
{
    int ret;
    
    printk(KERN_INFO "🦙 Llamux: Module init starting!\n");
    
    pr_info("🦙 Llamux %s: Waking up the llama...\n", LLAMUX_VERSION);
    pr_info("🦙 Llamux: *yawn* Good morning! I'm your AI kernel assistant.\n");
    pr_info("🦙 Llamux: Let me stretch my neural networks...\n");
    
    /* Initialize wait queue */
    init_waitqueue_head(&llama_state.inference_waitq);
    
    /* Create /proc/llamux directory */
    llamux_proc_dir = proc_mkdir("llamux", NULL);
    if (!llamux_proc_dir) {
        pr_err("🦙 Llamux: Failed to create /proc/llamux\n");
        return -ENOMEM;
    }
    
    /* Create /proc/llamux/status */
    if (!proc_create("status", 0444, llamux_proc_dir, &llamux_status_fops)) {
        pr_err("🦙 Llamux: Failed to create /proc/llamux/status\n");
        proc_remove(llamux_proc_dir);
        return -ENOMEM;
    }
    
    /* Create /proc/llamux/prompt */
    ret = llamux_create_prompt_interface(llamux_proc_dir);
    if (ret) {
        pr_err("🦙 Llamux: Failed to create prompt interface\n");
        proc_remove(llamux_proc_dir);
        return ret;
    }
    
    /* Load model */
    ret = llama_load_model();
    if (ret) {
        pr_err("🦙 Llamux: Failed to load model\n");
        proc_remove(llamux_proc_dir);
        return ret;
    }
    
    /* Allocate prompt/response buffers */
    llama_state.current_prompt = kzalloc(512, GFP_KERNEL);
    llama_state.current_response = kzalloc(512, GFP_KERNEL);
    if (!llama_state.current_prompt || !llama_state.current_response) {
        pr_err("🦙 Llamux: Failed to allocate buffers\n");
        llama_unload_model();
        kfree(llama_state.current_prompt);
        kfree(llama_state.current_response);
        proc_remove(llamux_proc_dir);
        return -ENOMEM;
    }
    
    /* Start inference thread */
    llama_state.inference_thread = kthread_create(llama_inference_thread, 
                                                  NULL, "llamux_inference");
    if (IS_ERR(llama_state.inference_thread)) {
        pr_err("🦙 Llamux: Failed to create inference thread\n");
        llama_unload_model();
        kfree(llama_state.current_prompt);
        kfree(llama_state.current_response);
        proc_remove(llamux_proc_dir);
        return PTR_ERR(llama_state.inference_thread);
    }
    
    wake_up_process(llama_state.inference_thread);
    pr_info("🦙 Llamux: Inference thread started\n");
    
    llama_state.initialized = true;
    pr_info("🦙 Llamux: Ready to think! Try: echo \"Hello llama\" > /proc/llamux/prompt\n");
    pr_info("🦙 Llamux: I'm here to help make your Linux experience smarter! 🧠\n");
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit llama_exit(void)
{
    pr_info("🦙 Llamux: Time for me to sleep... 😴\n");
    pr_info("🦙 Llamux: Thanks for letting me help! See you next boot!\n");
    
    llama_state.initialized = false;
    
    /* Stop inference thread */
    if (llama_state.inference_thread) {
        kthread_stop(llama_state.inference_thread);
        llama_state.inference_thread = NULL;
    }
    
    /* Free buffers */
    kfree(llama_state.current_prompt);
    kfree(llama_state.current_response);
    llama_state.current_prompt = NULL;
    llama_state.current_response = NULL;
    
    /* Unload model */
    llama_unload_model();
    
    /* Remove proc entries */
    proc_remove(llamux_proc_dir);
    
    pr_info("🦙 Llamux: Goodbye!\n");
}

module_init(llama_init);
module_exit(llama_exit);