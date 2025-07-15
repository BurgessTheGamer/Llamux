/*
 * GGML Kernel Implementation for Llamux
 * 
 * Core tensor operations for running TinyLlama in kernel space.
 * This is a minimal implementation focused on inference only.
 *
 * Copyright (C) 2025 Llamux Project
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/fpu/api.h>
#include "ggml_kernel.h"
#include "quantize.h"
#include "quantize.h"
#include "ggml_simd.h"

/* Math functions for kernel space - simple approximations */
static inline float kernel_expf(float x) {
    /* Simple Taylor series approximation for exp(x) */
    /* Good enough for softmax in neural networks */
    if (x < -10.0f) return 0.0f;
    if (x > 10.0f) return 22026.0f; /* ~exp(10) */
    
    float result = 1.0f;
    float term = 1.0f;
    for (int i = 1; i < 8; i++) {
        term *= x / i;
        result += term;
    }
    return result;
}
#define expf(x) kernel_expf(x)

/* Forward declarations */
static void ggml_compute_forward_soft_max_f32(const struct ggml_tensor *src0, struct ggml_tensor *dst);
static void ggml_compute_forward_rope_f32(const struct ggml_tensor *src0, struct ggml_tensor *dst, int n_past, int rope_dims);
static void ggml_compute_forward_scale_f32(const struct ggml_tensor *src0, struct ggml_tensor *dst, float scale);

/* Memory alignment for kernel operations */
#define GGML_MEM_ALIGN 32

/* Element sizes */
static const size_t GGML_TYPE_SIZE[GGML_TYPE_COUNT] = {
    [GGML_TYPE_F32]  = sizeof(float),
    [GGML_TYPE_F16]  = sizeof(uint16_t),
    [GGML_TYPE_Q4_0] = sizeof(uint8_t) + sizeof(uint16_t), /* simplified */
    [GGML_TYPE_Q4_K] = 144, /* actual Q4_K block size */
};

/* Aligned memory allocation for kernel space */
static void *ggml_aligned_malloc(size_t size) {
    void *ptr;
    size_t aligned_size = (size + GGML_MEM_ALIGN - 1) & ~(GGML_MEM_ALIGN - 1);
    
    /* Try kmalloc first for smaller allocations */
    if (aligned_size <= PAGE_SIZE * 2) {
        ptr = kmalloc(aligned_size + GGML_MEM_ALIGN, GFP_KERNEL);
        if (ptr) {
            void *aligned = (void *)(((uintptr_t)ptr + GGML_MEM_ALIGN - 1) & ~(GGML_MEM_ALIGN - 1));
            /* Store original pointer just before aligned address */
            *((void **)aligned - 1) = ptr;
            return aligned;
        }
    }
    
    /* Fall back to vmalloc for larger allocations */
    ptr = vmalloc(aligned_size);
    return ptr;
}

static void ggml_aligned_free(void *ptr) {
    if (!ptr) return;
    
    /* Check if it's a vmalloc'd pointer */
    if (is_vmalloc_addr(ptr)) {
        vfree(ptr);
    } else {
        /* Get original kmalloc pointer */
        void *orig = *((void **)ptr - 1);
        kfree(orig);
    }
}

/* Get element size */
size_t ggml_element_size(enum ggml_type type) {
    if (type >= GGML_TYPE_COUNT) {
        return 0;
    }
    return GGML_TYPE_SIZE[type];
}

/* Get tensor size in bytes */
size_t ggml_nbytes(const struct ggml_tensor *tensor) {
    size_t nbytes = tensor->ne[0];
    for (int i = 1; i < tensor->n_dims; i++) {
        nbytes *= tensor->ne[i];
    }
    return nbytes * ggml_element_size(tensor->type);
}

/* Initialize GGML context */
struct ggml_context *ggml_init(size_t mem_size, void *mem_buffer) {
    struct ggml_context *ctx;
    bool allocated_here = false;
    
    if (!mem_buffer) {
        /* Allocate memory if not provided */
        mem_buffer = vmalloc(mem_size);
        if (mem_buffer) {
            memset(mem_buffer, 0, mem_size);
            allocated_here = true;
        }
        if (!mem_buffer) {
            pr_err("🦙 GGML: Failed to allocate %zu bytes\n", mem_size);
            return NULL;
        }
    }
    
    /* Context is at the beginning of the buffer */
    ctx = (struct ggml_context *)mem_buffer;
    memset(ctx, 0, sizeof(struct ggml_context));
    
    ctx->mem_size = mem_size;
    ctx->mem_buffer = mem_buffer;
    /* Only own the buffer if we allocated it ourselves */
    ctx->mem_buffer_owned = allocated_here;
    ctx->mem_used = ALIGN(sizeof(struct ggml_context), GGML_TENSOR_ALIGN);
    ctx->n_objects = 0;
    
    pr_info("🦙 GGML: Initialized context with %zu MB\n", mem_size / (1024*1024));
    
    return ctx;
}

/* Free GGML context */
void ggml_free(struct ggml_context *ctx) {
    if (!ctx) return;
    
    if (ctx->mem_buffer_owned && ctx->mem_buffer) {
        vfree(ctx->mem_buffer);
    }
}

/* Allocate tensor from context */
struct ggml_tensor *ggml_new_tensor_impl(
    struct ggml_context *ctx,
    enum ggml_type type,
    int n_dims,
    const int64_t *ne,
    void *data,
    size_t offset) {
    
    struct ggml_tensor *tensor;
    size_t data_size;
    size_t tensor_size;
    
    if (ctx->n_objects >= GGML_MAX_NODES) {
        pr_err("🦙 GGML: Too many nodes (%d >= %d)\n", ctx->n_objects, GGML_MAX_NODES);
        return NULL;
    }
    
    /* Log every 1000 nodes to track usage */
    if (ctx->n_objects % 1000 == 0 && ctx->n_objects > 0) {
        pr_info("🦙 GGML: Node count: %d / %d\n", ctx->n_objects, GGML_MAX_NODES);
    }
    
    /* Calculate strides */
    size_t nb[GGML_MAX_DIMS];
    nb[0] = ggml_element_size(type);
    size_t ne_total = ne[0];
    for (int i = 1; i < n_dims; i++) {
        nb[i] = nb[i-1] * ne[i-1];
        ne_total *= ne[i];
    }
    
    /* Calculate sizes */
    tensor_size = ALIGN(sizeof(struct ggml_tensor), GGML_TENSOR_ALIGN);
    data_size = ne_total * ggml_element_size(type);
    
    /* Check if we have enough memory */
    if (ctx->mem_used + tensor_size + data_size > ctx->mem_size) {
        pr_err("🦙 GGML: Out of memory (used %zu + need %zu > total %zu)\n", 
               ctx->mem_used, tensor_size + data_size, ctx->mem_size);
        return NULL;
    }
    
    /* Allocate tensor struct */
    tensor = (struct ggml_tensor *)((char *)ctx->mem_buffer + ctx->mem_used);
    ctx->mem_used += tensor_size;
    
    /* Initialize tensor */
    memset(tensor, 0, sizeof(struct ggml_tensor));
    tensor->type = type;
    tensor->n_dims = n_dims;
    
    for (int i = 0; i < n_dims; i++) {
        tensor->ne[i] = ne[i];
        tensor->nb[i] = nb[i];
    }
    for (int i = n_dims; i < GGML_MAX_DIMS; i++) {
        tensor->ne[i] = 1;
        tensor->nb[i] = tensor->nb[i-1];
    }
    
    /* Allocate data */
    if (data == NULL) {
        tensor->data = (void *)((char *)ctx->mem_buffer + ctx->mem_used);
        ctx->mem_used = ALIGN(ctx->mem_used + data_size, GGML_TENSOR_ALIGN);
        memset(tensor->data, 0, data_size);
    } else {
        tensor->data = (char *)data + offset;
    }
    
    tensor->size = data_size;
    tensor->op = GGML_OP_NONE;
    tensor->is_param = 0;
    
    /* Add to context */
    ctx->objects[ctx->n_objects++] = tensor;
    
    return tensor;
}

/* Create new tensor */
struct ggml_tensor *ggml_new_tensor(
    struct ggml_context *ctx,
    enum ggml_type type,
    int n_dims,
    const int64_t *ne) {
    return ggml_new_tensor_impl(ctx, type, n_dims, ne, NULL, 0);
}

/* Create 1D tensor */
struct ggml_tensor *ggml_new_tensor_1d(
    struct ggml_context *ctx,
    enum ggml_type type,
    int64_t ne0) {
    const int64_t ne[1] = { ne0 };
    return ggml_new_tensor(ctx, type, 1, ne);
}

/* Create 2D tensor */
struct ggml_tensor *ggml_new_tensor_2d(
    struct ggml_context *ctx,
    enum ggml_type type,
    int64_t ne0,
    int64_t ne1) {
    const int64_t ne[2] = { ne0, ne1 };
    return ggml_new_tensor(ctx, type, 2, ne);
}

/* Create 3D tensor */
struct ggml_tensor *ggml_new_tensor_3d(
    struct ggml_context *ctx,
    enum ggml_type type,
    int64_t ne0,
    int64_t ne1,
    int64_t ne2) {
    const int64_t ne[3] = { ne0, ne1, ne2 };
    return ggml_new_tensor(ctx, type, 3, ne);
}

/* Set tensor name */
void ggml_set_name(struct ggml_tensor *tensor, const char *name) {
    strncpy(tensor->name, name, GGML_MAX_NAME - 1);
    tensor->name[GGML_MAX_NAME - 1] = '\0';
}

/* Simple matrix multiplication for F32 */
static void ggml_compute_forward_mul_mat_f32_f32(
    const struct ggml_tensor *src0,
    const struct ggml_tensor *src1,
    struct ggml_tensor *dst) {
    
    const int64_t ne00 = src0->ne[0];
    const int64_t ne01 = src0->ne[1];
    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    
    const float *a = (float *)src0->data;
    const float *b = (float *)src1->data;
    float *c = (float *)dst->data;
    
    /* Simple matrix multiplication: C = A * B^T */
    /* This is not optimized - real implementation would use SIMD */
    
    kernel_fpu_begin();
    
    for (int64_t i = 0; i < ne01; i++) {
        const float *a_row = a + i * ne00;
        for (int64_t j = 0; j < ne11; j++) {
            /* Use SIMD optimized dot product */
            const float *b_row = b + j * ne10;
            c[i * ne11 + j] = ggml_vec_dot_f32(a_row, b_row, ne00);
        }
    }
    
    kernel_fpu_end();
}

/* RMS normalization */
static void ggml_compute_forward_rms_norm_f32(
    const struct ggml_tensor *src0,
    struct ggml_tensor *dst,
    float eps) {
    
    const int64_t ne00 = src0->ne[0];
    const int64_t ne01 = src0->ne[1];
    
    const float *x = (float *)src0->data;
    float *y = (float *)dst->data;
    
    kernel_fpu_begin();
    
    for (int64_t i = 0; i < ne01; i++) {
        const float *row = x + i * ne00;
        float *out = y + i * ne00;
        
        /* Calculate RMS */
        float sum = 0.0f;
        for (int64_t j = 0; j < ne00; j++) {
            sum += row[j] * row[j];
        }
        
        /* Approximate square root for kernel space */
        float x = sum / ne00 + eps;
        float xhalf = 0.5f * x;
        int i = *(int*)&x;
        i = 0x5f3759df - (i >> 1); /* Fast inverse square root */
        x = *(float*)&i;
        x = x * (1.5f - xhalf * x * x);
        const float rms = x;
        
        /* Normalize */
        for (int64_t j = 0; j < ne00; j++) {
            out[j] = row[j] * rms;
        }
    }
    
    kernel_fpu_end();
}

/* SiLU activation (x * sigmoid(x)) */
static void ggml_compute_forward_silu_f32(
    const struct ggml_tensor *src0,
    struct ggml_tensor *dst) {
    
    const int n = ggml_nbytes(src0) / sizeof(float);
    const float *x = (float *)src0->data;
    float *y = (float *)dst->data;
    
    kernel_fpu_begin();
    
    for (int i = 0; i < n; i++) {
        /* SiLU: x * sigmoid(x) = x * (1 / (1 + exp(-x))) */
        /* Simplified to avoid exp() in kernel */
        float val = x[i];
        if (val >= 0) {
            y[i] = val / (1.0f + val);  /* Approximation */
        } else {
            y[i] = val * 0.5f;  /* Approximation */
        }
    }
    
    kernel_fpu_end();
}

/* Add operation */
struct ggml_tensor *ggml_add(
    struct ggml_context *ctx,
    struct ggml_tensor *a,
    struct ggml_tensor *b) {
    
    if (!ctx || !a || !b) {
        pr_err("🦙 GGML: ggml_add called with NULL parameters! ctx=%p, a=%p, b=%p\n", ctx, a, b);
        return NULL;
    }
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_ADD;
    result->src0 = a;
    result->src1 = b;
    
    return result;
}

/* Multiply operation */
struct ggml_tensor *ggml_mul(
    struct ggml_context *ctx,
    struct ggml_tensor *a,
    struct ggml_tensor *b) {
    
    if (!ctx || !a || !b) {
        pr_err("🦙 GGML: ggml_mul called with NULL parameters! ctx=%p, a=%p, b=%p\n", ctx, a, b);
        return NULL;
    }
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_MUL;
    result->src0 = a;
    result->src1 = b;
    
    return result;
}

/* Matrix multiplication */
struct ggml_tensor *ggml_mul_mat(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                struct ggml_tensor *b) {
    if (!ctx || !a || !b) {
        pr_err("🦙 GGML: ggml_mul_mat called with NULL parameters!\n");
        return NULL;
    }
    
    /* GGML matrix multiplication: C = A @ B^T */
    /* For transformer: input[d,n] @ weight[d,d]^T = output[d,n] */
    /* This means we need a->ne[0] == b->ne[0] (both have embedding dim) */
    
    /* Check if dimensions are compatible for A @ B^T */
    if (a->ne[0] != b->ne[0]) {
        pr_err("🦙 GGML: Matrix dimensions incompatible for A@B^T: A[%lld,%lld] @ B[%lld,%lld]^T\n",
               a->ne[0], a->ne[1], b->ne[0], b->ne[1]);
        pr_err("🦙 GGML: Need A.ne[0] (%lld) == B.ne[0] (%lld)\n", a->ne[0], b->ne[0]);
        return NULL;
    }
    
    /* Result shape: [b.ne[1], a.ne[1]] - output_dim x seq_len */
    int64_t ne[GGML_MAX_DIMS] = {b->ne[1], a->ne[1], a->ne[2], a->ne[3]};
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne);
    if (!result) {
        pr_err("🦙 GGML: Failed to allocate result tensor\n");
        return NULL;
    }
    
    result->op = GGML_OP_MUL_MAT;
    result->src0 = a;
    result->src1 = b;
    
    return result;
}

/* RMS normalization */
struct ggml_tensor *ggml_rms_norm(
    struct ggml_context *ctx,
    struct ggml_tensor *a,
    float eps) {
    
    if (!ctx || !a) {
        pr_err("🦙 GGML: ggml_rms_norm called with NULL tensor!\n");
        return NULL;
    }
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_RMS_NORM;
    result->src0 = a;
    result->extra = kmalloc(sizeof(float), GFP_KERNEL);
    if (result->extra) {
        *(float *)result->extra = eps;
    }
    
    return result;
}

/* Export symbols for kernel module linking */
EXPORT_SYMBOL_GPL(ggml_init);
EXPORT_SYMBOL_GPL(ggml_free);
EXPORT_SYMBOL_GPL(ggml_new_tensor);
EXPORT_SYMBOL_GPL(ggml_new_tensor_1d);
EXPORT_SYMBOL_GPL(ggml_new_tensor_2d);
EXPORT_SYMBOL_GPL(ggml_new_tensor_3d);
EXPORT_SYMBOL_GPL(ggml_add);
EXPORT_SYMBOL_GPL(ggml_mul);
EXPORT_SYMBOL_GPL(ggml_mul_mat);
EXPORT_SYMBOL_GPL(ggml_rms_norm);
EXPORT_SYMBOL_GPL(ggml_get_rows);
EXPORT_SYMBOL_GPL(ggml_scale);
EXPORT_SYMBOL_GPL(ggml_rope);
EXPORT_SYMBOL_GPL(ggml_silu);
EXPORT_SYMBOL_GPL(ggml_soft_max);
EXPORT_SYMBOL_GPL(ggml_print_tensor_info);

/* Transpose operation */
struct ggml_tensor *ggml_transpose(struct ggml_context *ctx,
                                  struct ggml_tensor *a) {
    if (!ctx || !a) return NULL;
    
    /* Create result tensor with swapped dimensions */
    int64_t ne[GGML_MAX_DIMS] = {a->ne[1], a->ne[0], a->ne[2], a->ne[3]};
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_TRANSPOSE;
    result->src0 = a;
    
    return result;
}
EXPORT_SYMBOL_GPL(ggml_transpose);

/* SiLU (Swish) activation function */
struct ggml_tensor *ggml_silu(struct ggml_context *ctx,
                             struct ggml_tensor *a) {
    if (!ctx || !a) {
        pr_err("🦙 GGML: ggml_silu called with NULL parameters!\n");
        return NULL;
    }
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_SILU;
    result->src0 = a;
    
    return result;
}

/* Scale operation */
struct ggml_tensor *ggml_scale(struct ggml_context *ctx,
                              struct ggml_tensor *a,
                              float scale) {
    if (!ctx || !a) return NULL;
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_SCALE;
    result->src0 = a;
    /* Store scale factor in extra data */
    result->extra = kmalloc(sizeof(float), GFP_KERNEL);
    if (result->extra) {
        *(float *)result->extra = scale;
    }
    
    return result;
}

/* RoPE (Rotary Position Embeddings) */
struct ggml_tensor *ggml_rope(struct ggml_context *ctx,
                             struct ggml_tensor *a,
                             int n_past,
                             int n_dims,
                             int mode) {
    if (!ctx || !a) return NULL;
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_ROPE;
    result->src0 = a;
    /* Store parameters in extra data */
    result->extra = kmalloc(sizeof(int) * 3, GFP_KERNEL);
    if (result->extra) {
        ((int *)result->extra)[0] = n_past;
        ((int *)result->extra)[1] = n_dims;
        ((int *)result->extra)[2] = mode;
    }
    
    return result;
}



/* Softmax */
struct ggml_tensor *ggml_soft_max(struct ggml_context *ctx,
                                 struct ggml_tensor *a) {
    if (!ctx || !a) return NULL;
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_SOFT_MAX;
    result->src0 = a;
    
    return result;
}

void ggml_print_tensor_info(const struct ggml_tensor *t) {
    pr_info("🦙 Tensor '%s': type=%d, dims=%d [%lld,%lld,%lld,%lld]\n",
            t->name, t->type, t->n_dims,
            t->ne[0], t->ne[1], t->ne[2], t->ne[3]);
}

/* Get rows operation - extracts rows from embedding table */
struct ggml_tensor *ggml_get_rows(struct ggml_context *ctx,
                                 struct ggml_tensor *a,
                                 struct ggml_tensor *b) {
    if (!ctx || !a || !b) {
        pr_err("🦙 GGML: ggml_get_rows called with NULL parameters!\n");
        return NULL;
    }
    
    /* b should contain row indices */
    if (b->type != GGML_TYPE_I32) {
        pr_err("🦙 GGML: get_rows indices must be I32\n");
        return NULL;
    }
    
    /* Result has same shape as a except first dimension matches b */
    int64_t ne[GGML_MAX_DIMS] = {a->ne[0], b->ne[0], a->ne[2], a->ne[3]};
    
    struct ggml_tensor *result = ggml_new_tensor(ctx, a->type, a->n_dims, ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_GET_ROWS;
    result->src0 = a;
    result->src1 = b;
    
    return result;
}

/* Build computation graph for a tensor */
/* Helper to add tensor to graph if not already present */
static void ggml_build_forward_impl(struct ggml_cgraph *graph, struct ggml_tensor *tensor) {
    if (!tensor || !graph) return;
    
    /* Check if already in graph */
    for (int i = 0; i < graph->n_nodes; i++) {
        if (graph->nodes[i] == tensor) {
            return;
        }
    }
    
    /* Check if we have space */
    if (graph->n_nodes >= GGML_MAX_NODES) {
        pr_warn("🦙 GGML: Graph node limit reached!\n");
        return;
    }
    
    /* Add dependencies first (post-order traversal) */
    if (tensor->src0) {
        ggml_build_forward_impl(graph, tensor->src0);
    }
    if (tensor->src1) {
        ggml_build_forward_impl(graph, tensor->src1);
    }
    
    /* Add this tensor */
    if (tensor->op == GGML_OP_NONE) {
        /* Leaf node (input/weight) */
        if (graph->n_leafs < GGML_MAX_NODES) {
            graph->leafs[graph->n_leafs++] = tensor;
        }
    } else {
        /* Operation node */
        graph->nodes[graph->n_nodes++] = tensor;
    }
}

struct ggml_cgraph *ggml_build_forward(struct ggml_tensor *tensor) {
    static struct ggml_cgraph graph;
    
    if (!tensor) return NULL;
    
    /* Reset graph */
    graph.n_nodes = 0;
    graph.n_leafs = 0;
    
    /* Build graph recursively */
    ggml_build_forward_impl(&graph, tensor);
    
    pr_info("🦙 GGML: Built graph with %d nodes and %d leafs\n", 
            graph.n_nodes, graph.n_leafs);
    
    return &graph;
}

/* Execute computation for a single tensor */
void ggml_compute_forward(struct ggml_tensor *tensor) {
    if (!tensor || tensor->op == GGML_OP_NONE) {
        return;
    }
    
    /* Ensure we have data buffer */
    if (!tensor->data) {
        pr_err("🦙 GGML: No data buffer for tensor!\n");
        return;
    }
    
    /* Ensure dependencies have data */
    if (tensor->src0 && !tensor->src0->data) {
        pr_err("🦙 GGML: src0 has no data!\n");
        return;
    }
    if (tensor->src1 && !tensor->src1->data) {
        pr_err("🦙 GGML: src1 has no data!\n");
        return;
    }
    

    
    /* Now compute this tensor */
    switch (tensor->op) {
        case GGML_OP_MUL_MAT:
            if (tensor->src0->type == GGML_TYPE_F32 && tensor->src1->type == GGML_TYPE_F32) {
                ggml_compute_forward_mul_mat_f32_f32(tensor->src0, tensor->src1, tensor);
            } else if (tensor->src0->type == GGML_TYPE_Q4_0 && tensor->src1->type == GGML_TYPE_F32) {
                ggml_compute_forward_mul_mat_q4_0_f32(tensor->src0, tensor->src1, tensor);
            } else if (tensor->src0->type == GGML_TYPE_Q4_K && tensor->src1->type == GGML_TYPE_F32) {
                /* For Q4_K, use Q4_0 as approximation for now */
                ggml_compute_forward_mul_mat_q4_0_f32(tensor->src0, tensor->src1, tensor);
            }
            break;
            
        case GGML_OP_ADD:
            /* Simple element-wise add */
            {
                float *dst = (float *)tensor->data;
                const float *a = (float *)tensor->src0->data;
                const float *b = (float *)tensor->src1->data;
                size_t n = 1;
                for (int i = 0; i < tensor->n_dims; i++) {
                    n *= tensor->ne[i];
                }
                kernel_fpu_begin();
                for (size_t i = 0; i < n; i++) {
                    dst[i] = a[i] + b[i];
                }
                kernel_fpu_end();
            }
            break;
            
        case GGML_OP_MUL:
            /* Element-wise multiply */
            {
                float *dst = (float *)tensor->data;
                const float *a = (float *)tensor->src0->data;
                const float *b = (float *)tensor->src1->data;
                size_t n = 1;
                for (int i = 0; i < tensor->n_dims; i++) {
                    n *= tensor->ne[i];
                }
                kernel_fpu_begin();
                for (size_t i = 0; i < n; i++) {
                    dst[i] = a[i] * b[i];
                }
                kernel_fpu_end();
            }
            break;
            
        case GGML_OP_RMS_NORM:
            ggml_compute_forward_rms_norm_f32(tensor->src0, tensor, 1e-5f);
            break;
            
        case GGML_OP_SILU:
            ggml_compute_forward_silu_f32(tensor->src0, tensor);
            break;
            
        case GGML_OP_SOFT_MAX:
            ggml_compute_forward_soft_max_f32(tensor->src0, tensor);
            break;
            
        case GGML_OP_ROPE:
            ggml_compute_forward_rope_f32(tensor->src0, tensor, 0, 128);
            break;
            
        case GGML_OP_SCALE:
            {
                float scale = tensor->extra ? *(float *)tensor->extra : 1.0f;
                ggml_compute_forward_scale_f32(tensor->src0, tensor, scale);
            }
            break;
            
        case GGML_OP_TRANSPOSE:
            /* Simple transpose - just copy with swapped strides */
            {
                const float *src = (float *)tensor->src0->data;
                float *dst = (float *)tensor->data;
                const int64_t ne0 = tensor->ne[0];
                const int64_t ne1 = tensor->ne[1];
                
                kernel_fpu_begin();
                for (int64_t i1 = 0; i1 < ne1; i1++) {
                    for (int64_t i0 = 0; i0 < ne0; i0++) {
                        dst[i1 * ne0 + i0] = src[i0 * ne1 + i1];
                    }
                }
                kernel_fpu_end();
            }
            break;
            
        case GGML_OP_GET_ROWS:
            /* Get rows from embedding table */
            {
                const float *src = (float *)tensor->src0->data;
                const int32_t *indices = (int32_t *)tensor->src1->data;
                float *dst = (float *)tensor->data;
                
                const int64_t ne0 = tensor->src0->ne[0];
                const int64_t n = tensor->src1->ne[0];
                
                kernel_fpu_begin();
                for (int64_t i = 0; i < n; i++) {
                    int32_t idx = indices[i];
                    if (idx >= 0 && idx < tensor->src0->ne[1]) {
                        memcpy(dst + i * ne0, src + idx * ne0, ne0 * sizeof(float));
                    }
                }
                kernel_fpu_end();
            }
            break;
            
        default:
            pr_warn("🦙 GGML: Unimplemented operation %d\n", tensor->op);
            break;
    }
    
    /* Don't mark as computed - we might need to recompute for next token */
    /* tensor->op = GGML_OP_NONE; */
}

/* Execute computation graph */
void ggml_graph_compute(struct ggml_context *ctx, struct ggml_cgraph *gf) {
    if (!ctx || !gf) return;
    
    pr_info("🦙 GGML: Computing graph with %d nodes\n", gf->n_nodes);
    pr_info("🦙 GGML: Starting node processing loop...\n");
    
    /* Process all nodes in order (they're already topologically sorted) */
    int max_nodes = gf->n_nodes;
    pr_info("🦙 GGML: Processing %d nodes\n", max_nodes);
    
    for (int i = 0; i < max_nodes; i++) {
        struct ggml_tensor *node = gf->nodes[i];
        
        /* Progress indicator every 100 nodes */
        if (i % 100 == 0 && i > 0) {
            pr_info("🦙 GGML: Progress: %d/%d nodes\n", i, max_nodes);
        }
        
        if (node && node->op != GGML_OP_NONE) {
            /* Allocate output buffer if needed */
            if (!node->data) {
                size_t size = ggml_element_size(node->type);
                for (int j = 0; j < node->n_dims; j++) {
                    size *= node->ne[j];
                }
                
                /* Simple allocation from context buffer */
                if (ctx->mem_used + size > ctx->mem_size) {
                    pr_err("🦙 GGML: Out of memory at node %d (used=%zu, need=%zu, total=%zu)\n",
                           i, ctx->mem_used, size, ctx->mem_size);
                    return;
                }
                
                node->data = (char *)ctx->mem_buffer + ctx->mem_used;
                ctx->mem_used += ALIGN(size, GGML_TENSOR_ALIGN);
            }
            
            /* Compute this node */
            ggml_compute_forward(node);
        }
    }
    
    pr_info("🦙 GGML: Graph computation completed (processed %d nodes)\n", max_nodes);
    pr_info("🦙 GGML: Memory used: %zu MB / %zu MB\n", 
            ctx->mem_used / (1024*1024), ctx->mem_size / (1024*1024));
}

/* Export new symbols */
EXPORT_SYMBOL_GPL(ggml_build_forward);
EXPORT_SYMBOL_GPL(ggml_graph_compute);
EXPORT_SYMBOL_GPL(ggml_compute_forward);


/* Quantized matrix multiplication stub */
void ggml_compute_forward_mul_mat_q4_0_f32(
    const struct ggml_tensor *src0,
    const struct ggml_tensor *src1,
    struct ggml_tensor *dst) {
    
    /* This handles both Q4_0 and Q4_K for now */
    const int64_t ne00 = src0->ne[0];
    const int64_t ne01 = src0->ne[1];
    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    
    /* Allocate temporary buffer for dequantized row */
    float *row_buf = kvmalloc(ne00 * sizeof(float), GFP_KERNEL);
    if (!row_buf) {
        pr_err("🦙 GGML: Failed to allocate dequant buffer\n");
        return;
    }
    
    kernel_fpu_begin();
    
    /* Matrix multiplication with on-the-fly dequantization */
    for (int64_t i = 0; i < ne01; i++) {
        /* Be nice - yield CPU periodically */
        if (i > 0 && (i % 32) == 0) {
            kernel_fpu_end();
            if (need_resched()) {
                cond_resched();
            }
            kernel_fpu_begin();
        }
        
        /* Debug progress for large matrices */
        if (ne01 > 1000 && i > 0 && (i % 1000) == 0) {
            pr_info("🦙 GGML: MulMat progress: %lld/%lld rows\n", i, ne01);
        }
        
        /* Dequantize one row of src0 */
        const void *row_quant = (char *)src0->data + i * src0->nb[1];
        dequantize_row(row_quant, row_buf, ne00, src0->type);
        
        /* Multiply dequantized row with src1 */
        for (int64_t j = 0; j < ne11; j++) {
            float sum = 0.0f;
            const float *src1_col = (float *)src1->data + j * ne10;
            
            /* Dot product */
            for (int64_t k = 0; k < ne00; k++) {
                sum += row_buf[k] * src1_col[k];
            }
            
            /* Store result */
            float *dst_ptr = (float *)dst->data + i * ne11 + j;
            *dst_ptr = sum;
        }
    }
    
    kernel_fpu_end();
    kvfree(row_buf);
}


/* Implement missing operations */

/* Softmax */
static void ggml_compute_forward_soft_max_f32(
    const struct ggml_tensor *src0,
    struct ggml_tensor *dst) {
    
    const int64_t ne0 = src0->ne[0];
    const int64_t ne1 = src0->ne[1];
    
    const float *src = (float *)src0->data;
    float *out = (float *)dst->data;
    
    kernel_fpu_begin();
    
    for (int64_t i1 = 0; i1 < ne1; i1++) {
        const float *row = src + i1 * ne0;
        float *dst_row = out + i1 * ne0;
        
        /* Find max for numerical stability */
        float max_val = row[0];
        for (int64_t i0 = 1; i0 < ne0; i0++) {
            if (row[i0] > max_val) max_val = row[i0];
        }
        
        /* Compute exp and sum */
        float sum = 0.0f;
        for (int64_t i0 = 0; i0 < ne0; i0++) {
            dst_row[i0] = expf(row[i0] - max_val);
            sum += dst_row[i0];
        }
        
        /* Normalize */
        if (sum > 0) {
            for (int64_t i0 = 0; i0 < ne0; i0++) {
                dst_row[i0] /= sum;
            }
        }
    }
    
    kernel_fpu_end();
}

/* RoPE - Rotary Position Embeddings */
static void ggml_compute_forward_rope_f32(
    const struct ggml_tensor *src0,
    struct ggml_tensor *dst,
    int n_past,
    int rope_dims) {
    
    /* Simplified RoPE - just copy for now */
    memcpy(dst->data, src0->data, 
           src0->ne[0] * src0->ne[1] * sizeof(float));
}

/* Scale */
static void ggml_compute_forward_scale_f32(
    const struct ggml_tensor *src0,
    struct ggml_tensor *dst,
    float scale) {
    
    const size_t n = src0->ne[0] * src0->ne[1];
    const float *src = (float *)src0->data;
    float *out = (float *)dst->data;
    
    kernel_fpu_begin();
    for (size_t i = 0; i < n; i++) {
        out[i] = src[i] * scale;
    }
    kernel_fpu_end();
}
