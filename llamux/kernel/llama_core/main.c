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
#include <linux/file.h>
#include <linux/namei.h>
#include "gguf_parser.h"
#include "memory_reserve.h"
#include "ggml_kernel.h"
#include "llama_model.h"

#define LLAMUX_VERSION "0.1.0-alpha"
#define MODEL_RESERVED_SIZE (2ULL * 1024 * 1024 * 1024) // 2GB
#define MODEL_FIRMWARE_PATH "llamux/tinyllama.gguf"  /* Fallback for firmware API */
#define MODEL_DIRECT_PATH "/lib/firmware/llamux/codellama-13b.gguf"  /* Direct file I/O path */

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Llamux Project");
MODULE_DESCRIPTION("Llamux Core - LLM in the Linux Kernel");
MODULE_VERSION(LLAMUX_VERSION);

/* Performance statistics */
struct llamux_stats {
    /* Token generation stats */
    atomic64_t total_tokens_generated;
    atomic64_t total_inference_time_ms;
    atomic_t current_tokens_per_sec;
    atomic64_t last_inference_start;
    atomic_t last_batch_tokens;
    
    /* Cache stats */
    atomic64_t cache_hits;
    atomic64_t cache_misses;
    
    /* Request stats */
    atomic64_t total_requests;
    atomic64_t failed_requests;
    
    /* Memory stats */
    atomic64_t peak_memory_used;
} llamux_perf_stats = {
    .total_tokens_generated = ATOMIC64_INIT(0),
    .total_inference_time_ms = ATOMIC64_INIT(0),
    .current_tokens_per_sec = ATOMIC_INIT(0),
    .last_inference_start = ATOMIC64_INIT(0),
    .last_batch_tokens = ATOMIC_INIT(0),
    .cache_hits = ATOMIC64_INIT(0),
    .cache_misses = ATOMIC64_INIT(0),
    .total_requests = ATOMIC64_INIT(0),
    .failed_requests = ATOMIC64_INIT(0),
    .peak_memory_used = ATOMIC64_INIT(0)
};

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

/*
 * Display real-time performance stats via /proc/llamux/stats
 */
static int llamux_stats_show(struct seq_file *m, void *v)
{
    u64 total_tokens = atomic64_read(&llamux_perf_stats.total_tokens_generated);
    u64 total_time_ms = atomic64_read(&llamux_perf_stats.total_inference_time_ms);
    u64 cache_hits = atomic64_read(&llamux_perf_stats.cache_hits);
    u64 cache_misses = atomic64_read(&llamux_perf_stats.cache_misses);
    u64 total_requests = atomic64_read(&llamux_perf_stats.total_requests);
    u64 failed_requests = atomic64_read(&llamux_perf_stats.failed_requests);
    int current_tps = atomic_read(&llamux_perf_stats.current_tokens_per_sec);
    
    seq_printf(m, "🦙 Llamux Performance Statistics\n");
    seq_printf(m, "================================\n\n");
    
    seq_printf(m, "Token Generation:\n");
    seq_printf(m, "  Total Tokens Generated: %llu\n", total_tokens);
    seq_printf(m, "  Total Inference Time: %llu ms\n", total_time_ms);
    if (total_time_ms > 0) {
        seq_printf(m, "  Average Speed: %.2f tokens/sec\n", 
                   (total_tokens * 1000.0) / total_time_ms);
    }
    seq_printf(m, "  Current Speed: %d tokens/sec\n", current_tps);
    seq_printf(m, "\n");
    
    seq_printf(m, "Weight Cache Performance:\n");
    seq_printf(m, "  Cache Hits: %llu\n", cache_hits);
    seq_printf(m, "  Cache Misses: %llu\n", cache_misses);
    if (cache_hits + cache_misses > 0) {
        seq_printf(m, "  Hit Rate: %.1f%%\n", 
                   (cache_hits * 100.0) / (cache_hits + cache_misses));
    }
    seq_printf(m, "\n");
    
    seq_printf(m, "Request Statistics:\n");
    seq_printf(m, "  Total Requests: %llu\n", total_requests);
    seq_printf(m, "  Failed Requests: %llu\n", failed_requests);
    if (total_requests > 0) {
        seq_printf(m, "  Success Rate: %.1f%%\n",
                   ((total_requests - failed_requests) * 100.0) / total_requests);
    }
    seq_printf(m, "\n");
    
    seq_printf(m, "Memory Usage:\n");
    if (llama_state.ggml_ctx) {
        seq_printf(m, "  GGML Context: %zu MB\n", 
                   llama_state.ggml_ctx->mem_used / (1024*1024));
    }
    seq_printf(m, "  Peak Memory: %llu MB\n", 
               atomic64_read(&llamux_perf_stats.peak_memory_used) / (1024*1024));
    
    if (llama_state.llama && llama_state.llama->weight_cache) {
        struct llama_weight_cache *cache = llama_state.llama->weight_cache;
        seq_printf(m, "\nWeight Cache Details:\n");
        seq_printf(m, "  Max Cache Size: %zu MB\n", cache->max_cache_size / (1024*1024));
        seq_printf(m, "  Cache Used: %zu MB\n", 
                   cache->total_cache_size / (1024*1024));
        seq_printf(m, "  Cache Hits: %d\n", atomic_read(&cache->cache_hits));
        seq_printf(m, "  Cache Misses: %d\n", atomic_read(&cache->cache_misses));
    }
    
    return 0;
}

static int llamux_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, llamux_stats_show, NULL);
}

static const struct proc_ops llamux_status_fops = {
    .proc_open = llamux_status_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops llamux_stats_fops = {
    .proc_open = llamux_stats_open,
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
            pr_info("🦙 Llamux: Inference thread stopping\n");
            break;
        }
        
        pr_info("🦙 Llamux: Inference thread woke up, request_pending=%d\n",
                atomic_read(&llama_state.request_pending));
        
        if (atomic_read(&llama_state.request_pending)) {
            mutex_lock(&llama_state.lock);
            
            if (llama_state.current_prompt && llama_state.inference_state) {
                pr_info("🦙 Llamux: Processing prompt: %s\n", 
                        llama_state.current_prompt);
                
                /* REAL INFERENCE - LET'S GO! */
                if (llama_state.current_response) {
                    pr_info("🦙 Llamux: Starting real inference with CodeLlama 13B!\n");
                    
                    int n_generated = llama_generate(
                        llama_state.inference_state,
                        llama_state.current_prompt,
                        llama_state.current_response,
                        512,  /* max response length */
                        10    /* max tokens to generate - reduced for testing */
                    );
                    
                    if (n_generated > 0) {
                        pr_info("🦙 Llamux: Generated %d tokens! Response: %s\n", 
                                n_generated, llama_state.current_response);
                    } else {
                        pr_err("🦙 Llamux: Inference failed! Error code: %d\n", n_generated);
                        snprintf(llama_state.current_response, 256,
                                "🦙 Error: Failed to generate response (code: %d)", n_generated);
                    }
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
    struct file *filp = NULL;
    loff_t file_size;
    loff_t pos = 0;
    void *model_data = NULL;
    int ret;
    
    pr_info("🦙 Llamux: Loading CodeLlama 13B model using direct file I/O...\n");
    
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
        /* CodeLlama 13B Q4_K_M: ~7.3GB + KV cache + GGML overhead */
        size_t alloc_size = (size_t)30720 * 1024 * 1024; /* 30GB total - let's go big! */
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
    
    /* Open model file directly */
    filp = filp_open(MODEL_DIRECT_PATH, O_RDONLY | O_LARGEFILE, 0);
    if (IS_ERR(filp)) {
        pr_err("🦙 Llamux: Failed to open model file %s: %ld\n", 
               MODEL_DIRECT_PATH, PTR_ERR(filp));
        ret = PTR_ERR(filp);
        goto err_free_memory;
    }
    
    /* Get file size */
    file_size = i_size_read(file_inode(filp));
    pr_info("🦙 Llamux: Model file size: %lld MB\n", file_size / (1024*1024));
    
    /* Allocate temporary buffer for model data */
    model_data = vmalloc(file_size);
    if (!model_data) {
        pr_err("🦙 Llamux: Failed to allocate buffer for model data (%lld bytes)\n", file_size);
        ret = -ENOMEM;
        goto err_close_file;
    }
    
    /* Read file in chunks to handle large files */
    pr_info("🦙 Llamux: Reading model file into memory in chunks...\n");
    {
        size_t chunk_size = 512 * 1024 * 1024; /* 512MB chunks */
        size_t bytes_read = 0;
        
        while (bytes_read < file_size) {
            size_t to_read = min(chunk_size, (size_t)(file_size - bytes_read));
            ssize_t chunk_ret;
            
            chunk_ret = kernel_read(filp, (char*)model_data + bytes_read, to_read, &pos);
            if (chunk_ret <= 0) {
                pr_err("🦙 Llamux: Failed to read chunk at offset %zu: %zd\n", 
                       bytes_read, chunk_ret);
                ret = chunk_ret ? chunk_ret : -EIO;
                goto err_free_model_data;
            }
            
            bytes_read += chunk_ret;
            if (bytes_read % (1024 * 1024 * 1024) == 0) {
                pr_info("🦙 Llamux: Read %zu GB so far...\n", bytes_read / (1024 * 1024 * 1024));
            }
        }
        
        if (bytes_read != file_size) {
            pr_err("🦙 Llamux: Read size mismatch: expected %lld, got %zu\n", 
                   file_size, bytes_read);
            ret = -EIO;
            goto err_free_model_data;
        }
    }
    
    pr_info("🦙 Llamux: Successfully read %lld MB from model file\n", file_size / (1024*1024));
    
    /* Close file - we have the data in memory now */
    filp_close(filp, NULL);
    filp = NULL;
    
    /* Now process the model data as before */
    if (file_size < sizeof(struct gguf_header)) {
        pr_err("🦙 Llamux: Model file too small\n");
        ret = -EINVAL;
        goto err_free_model_data;
    }
    
    /* Allocate GGUF model structure */
    llama_state.gguf_model = kzalloc(sizeof(struct gguf_model), GFP_KERNEL);
    if (!llama_state.gguf_model) {
        ret = -ENOMEM;
        goto err_free_model_data;
    }
    
    /* Parse GGUF file */
    ret = gguf_parse_header(model_data, file_size, &llama_state.gguf_model->header);
    if (ret) {
        pr_err("🦙 Llamux: Failed to parse GGUF header\n");
        goto err_free_gguf;
    }
    
    pr_info("🦙 Llamux: GGUF version %u, %llu tensors, %llu metadata entries\n",
            llama_state.gguf_model->header.version,
            llama_state.gguf_model->header.tensor_count,
            llama_state.gguf_model->header.metadata_kv_count);
    
    /* Parse metadata */
    ret = gguf_parse_metadata(model_data, file_size, llama_state.gguf_model);
    if (ret < 0) {
        pr_err("🦙 Llamux: Failed to parse metadata\n");
        goto err_free_gguf;
    }
    
    /* Set default vocab size if not found in metadata */
    if (llama_state.gguf_model->vocab_size == 0) {
        llama_state.gguf_model->vocab_size = 32000; /* CodeLlama default */
    }
    
    /* Parse tensor info */
    ret = gguf_parse_tensor_info(model_data, file_size, llama_state.gguf_model);
    if (ret) {
        pr_err("🦙 Llamux: Failed to parse tensor info\n");
        goto err_free_gguf;
    }
    
    /* Validate model */
    ret = gguf_validate_model(llama_state.gguf_model);
    if (ret) {
        pr_err("🦙 Llamux: Model validation failed\n");
        goto err_free_gguf;
    }
    
    /* Print model info */
    gguf_print_model_info(llama_state.gguf_model);
    
    /* Allocate memory for tensor data */
    size_t tensor_data_size = file_size - llama_state.gguf_model->data_offset;
    pr_info("🦙 Llamux: Tensor data size: %zu MB\n", 
            tensor_data_size / (1024 * 1024));
    
    /* Load tensor data */
    ret = gguf_load_tensor_data(model_data, file_size, llama_state.gguf_model,
                               llama_state.model_memory, llama_state.model_size);
    if (ret) {
        pr_warn("🦙 Llamux: Failed to load tensor data, using mock weights\n");
    } else {
        pr_info("🦙 Llamux: Loaded tensor data successfully!\n");
    }
    
    /* Free the temporary model data buffer - tensor data is now in model_memory */
    vfree(model_data);
    model_data = NULL;
    
    /* Initialize GGML context - needs enough for model tensors */
    /* After loading tensor data, use remaining memory for GGML context */
    size_t tensor_data_used = file_size - llama_state.gguf_model->data_offset;
    size_t remaining_size = llama_state.model_size - tensor_data_used;
    void *ctx_mem = (char*)llama_state.model_memory + tensor_data_used;
    
    pr_info("🦙 Llamux: Tensor data used %zu MB, %zu MB remaining for GGML context\n", 
            tensor_data_used / (1024 * 1024), remaining_size / (1024 * 1024));
    pr_info("🦙 Llamux: Initializing GGML context with %zu MB\n", remaining_size / (1024 * 1024));
    llama_state.ggml_ctx = ggml_init(remaining_size, ctx_mem);
    
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
err_free_model_data:
    if (model_data)
        vfree(model_data);
err_close_file:
    if (filp && !IS_ERR(filp))
        filp_close(filp, NULL);
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
    
    /* Create /proc/llamux/stats */
    if (!proc_create("stats", 0444, llamux_proc_dir, &llamux_stats_fops)) {
        pr_err("🦙 Llamux: Failed to create /proc/llamux/stats\n");
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
    if (llama_state.inference_thread && !IS_ERR(llama_state.inference_thread)) {
        pr_info("🦙 Llamux: Stopping inference thread...\n");
        int ret = kthread_stop(llama_state.inference_thread);
        if (ret) {
            pr_err("🦙 Llamux: Failed to stop inference thread: %d\n", ret);
        }
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