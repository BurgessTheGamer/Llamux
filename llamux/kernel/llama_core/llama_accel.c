/*
 * Llamux Acceleration Engine Implementation
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched/isolation.h>
#include <linux/sched/task.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/hugetlb.h>
#include <linux/list.h>
#include <asm/fpu/api.h>
#include <asm/msr.h>

#include "llama_accel.h"
#include "ggml_kernel.h"
#include "quantize.h"

/* Global acceleration engine */
struct llama_accel_engine *llama_accel = NULL;

/* Forward declarations */
static int llama_compute_thread_fn(void *data);
static void llama_process_request(struct llama_compute_request *req);

/*
 * Memory pool management
 */
static struct llama_mem_pool *llama_mem_pool_create(size_t size, const char *name) {
    struct llama_mem_pool *pool;
    int nr_pages;
    
    pool = kzalloc(sizeof(*pool), GFP_KERNEL);
    if (!pool)
        return NULL;
    
    /* Try to allocate 1GB huge pages first */
    nr_pages = size / HUGE_PAGE_SIZE;
    if (nr_pages > 0) {
        pool->base_addr = vmalloc_huge(size, GFP_KERNEL);
        if (pool->base_addr) {
            pr_info("🦙 Accel: Allocated %zu MB using huge pages for %s\n",
                    size / (1024 * 1024), name);
            pool->nr_hugepages = nr_pages;
        }
    }
    
    /* Fall back to regular vmalloc */
    if (!pool->base_addr) {
        pool->base_addr = vmalloc(size);
        if (!pool->base_addr) {
            kfree(pool);
            return NULL;
        }
        pr_info("🦙 Accel: Allocated %zu MB using regular pages for %s\n",
                size / (1024 * 1024), name);
    }
    
    pool->size = size;
    pool->used = 0;
    mutex_init(&pool->lock);
    
    return pool;
}

static void llama_mem_pool_destroy(struct llama_mem_pool *pool) {
    if (!pool)
        return;
    
    if (pool->base_addr)
        vfree(pool->base_addr);
    
    kfree(pool);
}

/*
 * CPU isolation and affinity
 */
static int llama_setup_compute_thread(struct llama_compute_thread *thread, int cpu) {
    
    thread->cpu_id = cpu;
    INIT_LIST_HEAD(&thread->work_list);
    spin_lock_init(&thread->work_lock);
    init_waitqueue_head(&thread->work_wait);
    atomic64_set(&thread->requests_processed, 0);
    atomic64_set(&thread->total_cycles, 0);
    
    /* Create high-priority kernel thread */
    thread->task = kthread_create(llama_compute_thread_fn, thread,
                                  "llama_compute_%d", cpu);
    if (IS_ERR(thread->task)) {
        pr_err("🦙 Accel: Failed to create compute thread for CPU %d\n", cpu);
        return PTR_ERR(thread->task);
    }
    
    /* Bind to specific CPU */
    kthread_bind(thread->task, cpu);
    
    /* Set high priority (kernel threads don't use RT scheduler) */
    set_user_nice(thread->task, -20);
    
    /* Start the thread */
    wake_up_process(thread->task);
    
    pr_info("🦙 Accel: Started compute thread on CPU %d with RT priority\n", cpu);
    
    return 0;
}

/*
 * Compute thread main function
 */
static int llama_compute_thread_fn(void *data) {
    struct llama_compute_thread *thread = data;
    struct llama_compute_request *req;
    unsigned long flags;
    
    pr_info("🦙 Accel: Compute thread started on CPU %d\n", thread->cpu_id);
    
    /* Disable preemption for this thread */
    set_current_state(TASK_INTERRUPTIBLE);
    
    while (!kthread_should_stop()) {
        /* Wait for work */
        wait_event_interruptible(thread->work_wait,
                                !list_empty(&thread->work_list) ||
                                kthread_should_stop());
        
        if (kthread_should_stop())
            break;
        
        /* Get next request */
        spin_lock_irqsave(&thread->work_lock, flags);
        if (!list_empty(&thread->work_list)) {
            /* For now, just get the first request pointer stored in context */
            struct list_head *entry = thread->work_list.next;
            list_del(entry);
            req = (struct llama_compute_request *)entry;
        } else {
            req = NULL;
        }
        spin_unlock_irqrestore(&thread->work_lock, flags);
        
        if (req) {
            u64 start_cycles = ktime_get_ns();
            
            /* Process the request */
            llama_process_request(req);
            
            u64 end_cycles = ktime_get_ns();
            atomic64_add(end_cycles - start_cycles, &thread->total_cycles);
            atomic64_inc(&thread->requests_processed);
            
            /* Call completion callback */
            if (req->complete)
                req->complete(req);
        }
    }
    
    pr_info("🦙 Accel: Compute thread on CPU %d stopping\n", thread->cpu_id);
    return 0;
}

/*
 * Process compute request
 */
static void llama_process_request(struct llama_compute_request *req) {
    switch (req->op) {
    case LLAMA_OP_MATMUL_Q4K:
        /* Use optimized matrix multiplication */
        kernel_fpu_begin();
        llama_accel_matmul_q4k(req->src0, req->src1, req->dst,
                              req->m, req->n, req->k);
        kernel_fpu_end();
        break;
        
    case LLAMA_OP_ATTENTION:
        /* Implement attention mechanism */
        pr_debug("🦙 Accel: Processing attention operation\n");
        break;
        
    case LLAMA_OP_SOFTMAX:
        /* Implement softmax */
        pr_debug("🦙 Accel: Processing softmax operation\n");
        break;
        
    default:
        pr_warn("🦙 Accel: Unknown operation %d\n", req->op);
        break;
    }
    
    req->complete_time = ktime_get_ns();
}

/*
 * Submit compute request
 */
int llama_accel_submit(struct llama_compute_request *req) {
    struct llama_compute_thread *thread;
    unsigned long flags;
    int cpu;
    
    if (!llama_accel || !llama_accel->initialized)
        return -ENODEV;
    
    req->submit_time = ktime_get_ns();
    
    /* Round-robin scheduling across compute threads */
    cpu = atomic_inc_return(&llama_accel->pending_requests) % 
          llama_accel->nr_compute_threads;
    thread = &llama_accel->threads[cpu];
    
    /* Add to thread's work queue - store request pointer in list entry */
    spin_lock_irqsave(&thread->work_lock, flags);
    list_add_tail((struct list_head *)req, &thread->work_list);
    spin_unlock_irqrestore(&thread->work_lock, flags);
    
    /* Wake up the thread */
    wake_up(&thread->work_wait);
    
    return 0;
}

/*
 * Optimized matrix multiplication for Q4_K
 */
void llama_accel_matmul_q4k(const void *A, const float *B,
                            float *C, int M, int N, int K) {
    const int nb = K / QK_K;
    const int TILE_SIZE = 32;  /* Renamed to avoid conflict */
    int i, j;
    
    /* Process in blocks for better cache usage */
    for (i = 0; i < M; i += TILE_SIZE) {
        for (j = 0; j < N; j += TILE_SIZE) {
            int bi, bj;
            
            /* Process block */
            for (bi = i; bi < min(i + TILE_SIZE, M); bi++) {
                const struct block_q4_K *row = 
                    (const struct block_q4_K *)A + bi * nb;
                
                for (bj = j; bj < min(j + TILE_SIZE, N); bj++) {
                    const float *col = B + bj * K;
                    float sum = 0.0f;
                    int k;
                    
                    /* Optimized dot product */
                    for (k = 0; k < nb; k++) {
                        const struct block_q4_K *block = &row[k];
                        const float *b = col + k * QK_K;
                        int l;
                        
                        /* Dequantize and compute */
                        for (l = 0; l < QK_K/2; l++) {
                            uint8_t byte = block->qs[l];
                            float v0 = (float)((byte & 0xF) - 8) * block->d;
                            float v1 = (float)(((byte >> 4) & 0xF) - 8) * block->d;
                            
                            sum += v0 * b[l*2];
                            sum += v1 * b[l*2 + 1];
                        }
                    }
                    
                    C[bi * N + bj] = sum;
                }
            }
        }
    }
}

/*
 * Initialize acceleration engine
 */
int llama_accel_init(const cpumask_t *compute_cpus) {
    int cpu, ret = 0;
    
    if (llama_accel) {
        pr_warn("🦙 Accel: Already initialized\n");
        return -EEXIST;
    }
    
    llama_accel = kzalloc(sizeof(*llama_accel), GFP_KERNEL);
    if (!llama_accel)
        return -ENOMEM;
    
    mutex_init(&llama_accel->init_lock);
    cpumask_copy(&llama_accel->compute_cpus, compute_cpus);
    
    /* Create memory pools */
    llama_accel->weight_pool = llama_mem_pool_create(8UL * 1024 * 1024 * 1024,
                                                     "weights");
    if (!llama_accel->weight_pool) {
        ret = -ENOMEM;
        goto err_free;
    }
    
    llama_accel->activation_pool = llama_mem_pool_create(2UL * 1024 * 1024 * 1024,
                                                         "activations");
    if (!llama_accel->activation_pool) {
        ret = -ENOMEM;
        goto err_free_weight;
    }
    
    /* Create compute threads */
    llama_accel->nr_compute_threads = 0;
    for_each_cpu(cpu, compute_cpus) {
        if (llama_accel->nr_compute_threads >= MAX_COMPUTE_THREADS)
            break;
        
        ret = llama_setup_compute_thread(
            &llama_accel->threads[llama_accel->nr_compute_threads], cpu);
        if (ret)
            goto err_stop_threads;
        
        llama_accel->nr_compute_threads++;
    }
    
    /* Create submit workqueue */
    llama_accel->submit_wq = alloc_workqueue("llama_submit",
                                            WQ_HIGHPRI | WQ_CPU_INTENSIVE,
                                            llama_accel->nr_compute_threads);
    if (!llama_accel->submit_wq) {
        ret = -ENOMEM;
        goto err_stop_threads;
    }
    
    llama_accel->initialized = true;
    
    pr_info("🦙 Accel: Initialized with %d compute threads on CPUs %*pbl\n",
            llama_accel->nr_compute_threads, cpumask_pr_args(compute_cpus));
    
    return 0;
    
err_stop_threads:
    for (cpu = 0; cpu < llama_accel->nr_compute_threads; cpu++) {
        if (llama_accel->threads[cpu].task)
            kthread_stop(llama_accel->threads[cpu].task);
    }
    llama_mem_pool_destroy(llama_accel->activation_pool);
err_free_weight:
    llama_mem_pool_destroy(llama_accel->weight_pool);
err_free:
    kfree(llama_accel);
    llama_accel = NULL;
    return ret;
}

/*
 * Cleanup acceleration engine
 */
void llama_accel_cleanup(void) {
    int i;
    
    if (!llama_accel)
        return;
    
    llama_accel->initialized = false;
    
    /* Stop all compute threads */
    for (i = 0; i < llama_accel->nr_compute_threads; i++) {
        if (llama_accel->threads[i].task)
            kthread_stop(llama_accel->threads[i].task);
    }
    
    /* Destroy workqueue */
    if (llama_accel->submit_wq)
        destroy_workqueue(llama_accel->submit_wq);
    
    /* Free memory pools */
    llama_mem_pool_destroy(llama_accel->weight_pool);
    llama_mem_pool_destroy(llama_accel->activation_pool);
    
    kfree(llama_accel);
    llama_accel = NULL;
    
    pr_info("🦙 Accel: Cleanup complete\n");
}

/*
 * Memory allocation from pools
 */
void *llama_accel_alloc(size_t size, bool is_weight) {
    struct llama_mem_pool *pool;
    void *ptr = NULL;
    
    if (!llama_accel || !llama_accel->initialized)
        return NULL;
    
    pool = is_weight ? llama_accel->weight_pool : llama_accel->activation_pool;
    
    mutex_lock(&pool->lock);
    if (pool->used + size <= pool->size) {
        ptr = pool->base_addr + pool->used;
        pool->used += ALIGN(size, 64);  /* 64-byte alignment */
    }
    mutex_unlock(&pool->lock);
    
    return ptr;
}

void llama_accel_free(void *ptr) {
    /* Pool allocations are not individually freed */
    /* They're freed when the pool is destroyed */
}