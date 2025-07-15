/*
 * Llamux Acceleration Engine
 * 
 * Custom acceleration for kernel-space LLM inference
 * Provides CPU isolation, dedicated compute threads, and optimized memory
 */

#ifndef _LLAMUX_ACCEL_H
#define _LLAMUX_ACCEL_H

#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/ring_buffer.h>
#include <linux/huge_mm.h>
#include <linux/dma-mapping.h>

#define MAX_COMPUTE_THREADS 16
#define COMPUTE_RING_SIZE 1024
#define HUGE_PAGE_SIZE (1UL << 30)  /* 1GB huge pages */

/* Compute request types */
enum llama_compute_op {
    LLAMA_OP_MATMUL_Q4K,
    LLAMA_OP_ATTENTION,
    LLAMA_OP_LAYERNORM,
    LLAMA_OP_SOFTMAX,
    LLAMA_OP_ROPE,
};

/* Compute request structure */
struct llama_compute_request {
    enum llama_compute_op op;
    void *src0;
    void *src1;
    void *dst;
    size_t m, n, k;  /* Matrix dimensions */
    
    /* Completion callback */
    void (*complete)(struct llama_compute_request *req);
    void *context;
    
    /* Performance tracking */
    u64 submit_time;
    u64 complete_time;
};

/* Memory pool for huge pages */
struct llama_mem_pool {
    void *base_addr;
    size_t size;
    size_t used;
    struct mutex lock;
    
    /* Huge page info */
    int nr_hugepages;
    struct page **pages;
};

/* Per-CPU compute thread data */
struct llama_compute_thread {
    struct task_struct *task;
    int cpu_id;
    
    /* Local work queue */
    struct list_head work_list;
    spinlock_t work_lock;
    wait_queue_head_t work_wait;
    
    /* Statistics */
    atomic64_t requests_processed;
    atomic64_t total_cycles;
};

/* Main acceleration engine */
struct llama_accel_engine {
    /* CPU management */
    cpumask_t compute_cpus;
    cpumask_t isolated_cpus;
    int nr_compute_threads;
    
    /* Compute threads */
    struct llama_compute_thread threads[MAX_COMPUTE_THREADS];
    
    /* Global request queue */
    struct workqueue_struct *submit_wq;
    atomic_t pending_requests;
    
    /* Memory pools */
    struct llama_mem_pool *weight_pool;
    struct llama_mem_pool *activation_pool;
    
    /* DMA engine (optional) */
    struct dma_chan *dma_chan;
    
    /* Performance monitoring */
    atomic64_t total_requests;
    atomic64_t total_compute_cycles;
    
    /* Engine state */
    bool initialized;
    struct mutex init_lock;
};

/* Global acceleration engine instance */
extern struct llama_accel_engine *llama_accel;

/* Initialization and cleanup */
int llama_accel_init(const cpumask_t *compute_cpus);
void llama_accel_cleanup(void);

/* Submit compute request */
int llama_accel_submit(struct llama_compute_request *req);

/* Memory allocation from huge page pools */
void *llama_accel_alloc(size_t size, bool is_weight);
void llama_accel_free(void *ptr);

/* CPU isolation control */
int llama_accel_isolate_cpus(const cpumask_t *cpus);
int llama_accel_release_cpus(const cpumask_t *cpus);

/* Performance monitoring */
void llama_accel_get_stats(struct llama_accel_stats *stats);

/* Optimized compute operations */
void llama_accel_matmul_q4k(const void *A, const float *B, 
                            float *C, int M, int N, int K);
void llama_accel_attention(const float *Q, const float *K,
                          const float *V, float *out, 
                          int seq_len, int d_head);

#endif /* _LLAMUX_ACCEL_H */