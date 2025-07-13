/*
 * GGML Kernel Implementation for Llamux
 * 
 * Minimal tensor operations for LLM inference in kernel space.
 * This is a highly simplified version of GGML focused on TinyLlama needs.
 *
 * Copyright (C) 2025 Llamux Project
 */

#ifndef _LLAMUX_GGML_KERNEL_H
#define _LLAMUX_GGML_KERNEL_H

#include <linux/types.h>
#include <linux/kernel.h>

/* Configuration */
#define GGML_MAX_DIMS      4
#define GGML_MAX_NODES     4096
#define GGML_MAX_NAME      64
#define GGML_TENSOR_ALIGN  32

/* Forward declarations */
struct ggml_context;
struct ggml_tensor;

/* Tensor type (simplified) */
enum ggml_type {
    GGML_TYPE_F32   = 0,
    GGML_TYPE_F16   = 1,
    GGML_TYPE_Q4_0  = 2,
    GGML_TYPE_Q4_K  = 12,
    GGML_TYPE_COUNT
};

/* Operations */
enum ggml_op {
    GGML_OP_NONE = 0,
    GGML_OP_DUP,
    GGML_OP_ADD,
    GGML_OP_SUB,
    GGML_OP_MUL,
    GGML_OP_DIV,
    GGML_OP_MUL_MAT,
    GGML_OP_NORM,
    GGML_OP_RMS_NORM,
    GGML_OP_SILU,
    GGML_OP_SOFT_MAX,
    GGML_OP_ROPE,
    GGML_OP_RESHAPE,
    GGML_OP_VIEW,
    GGML_OP_PERMUTE,
    GGML_OP_GET_ROWS,
    GGML_OP_CPY,
    GGML_OP_CONT,
    GGML_OP_COUNT
};

/* Tensor structure */
struct ggml_tensor {
    enum ggml_type type;
    
    int n_dims;
    int64_t ne[GGML_MAX_DIMS];     /* number of elements */
    size_t  nb[GGML_MAX_DIMS];     /* stride in bytes */
    
    /* Operation */
    enum ggml_op op;
    struct ggml_tensor *src0;
    struct ggml_tensor *src1;
    
    /* Data */
    void *data;
    size_t size;
    
    /* Name for debugging */
    char name[GGML_MAX_NAME];
    
    /* Computation flags */
    int is_param;
    
    /* Extra data for operations */
    void *extra;
};

/* Context for memory allocation */
struct ggml_context {
    size_t mem_size;
    void  *mem_buffer;
    bool   mem_buffer_owned;
    
    int    n_objects;
    struct ggml_tensor *objects[GGML_MAX_NODES];
    
    /* Simple bump allocator */
    size_t mem_used;
};

/* Computation plan */
struct ggml_cgraph {
    int n_nodes;
    int n_leafs;
    
    struct ggml_tensor *nodes[GGML_MAX_NODES];
    struct ggml_tensor *grads[GGML_MAX_NODES];
    struct ggml_tensor *leafs[GGML_MAX_NODES];
};

/* Basic functions */
struct ggml_context *ggml_init(size_t mem_size, void *mem_buffer);
void ggml_free(struct ggml_context *ctx);

/* Tensor creation */
struct ggml_tensor *ggml_new_tensor(struct ggml_context *ctx,
                                   enum ggml_type type,
                                   int n_dims,
                                   const int64_t *ne);

struct ggml_tensor *ggml_new_tensor_1d(struct ggml_context *ctx,
                                       enum ggml_type type,
                                       int64_t ne0);

struct ggml_tensor *ggml_new_tensor_2d(struct ggml_context *ctx,
                                       enum ggml_type type,
                                       int64_t ne0,
                                       int64_t ne1);

struct ggml_tensor *ggml_new_tensor_3d(struct ggml_context *ctx,
                                       enum ggml_type type,
                                       int64_t ne0,
                                       int64_t ne1,
                                       int64_t ne2);

/* Basic operations */
struct ggml_tensor *ggml_add(struct ggml_context *ctx,
                            struct ggml_tensor *a,
                            struct ggml_tensor *b);

struct ggml_tensor *ggml_mul(struct ggml_context *ctx,
                            struct ggml_tensor *a,
                            struct ggml_tensor *b);

struct ggml_tensor *ggml_mul_mat(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                struct ggml_tensor *b);

struct ggml_tensor *ggml_rms_norm(struct ggml_context *ctx,
                                 struct ggml_tensor *a,
                                 float eps);

struct ggml_tensor *ggml_soft_max(struct ggml_context *ctx,
                                  struct ggml_tensor *a);

struct ggml_tensor *ggml_silu(struct ggml_context *ctx,
                             struct ggml_tensor *a);

struct ggml_tensor *ggml_rope(struct ggml_context *ctx,
                             struct ggml_tensor *a,
                             int n_past,
                             int n_dims,
                             int mode);

/* View operations */
struct ggml_tensor *ggml_view_1d(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                int64_t ne0,
                                size_t offset);

struct ggml_tensor *ggml_view_2d(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                int64_t ne0,
                                int64_t ne1,
                                size_t nb1,
                                size_t offset);

struct ggml_tensor *ggml_reshape(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                struct ggml_tensor *b);

struct ggml_tensor *ggml_permute(struct ggml_context *ctx,
                                struct ggml_tensor *a,
                                int axis0,
                                int axis1,
                                int axis2,
                                int axis3);

/* Compute operations */
void ggml_compute_forward(struct ggml_tensor *tensor);
struct ggml_cgraph *ggml_build_forward(struct ggml_tensor *tensor);
void ggml_graph_compute(struct ggml_context *ctx, struct ggml_cgraph *gf);

/* Utility functions */
size_t ggml_element_size(enum ggml_type type);
size_t ggml_tensor_overhead(void);
size_t ggml_nbytes(const struct ggml_tensor *tensor);
void   ggml_set_name(struct ggml_tensor *tensor, const char *name);

/* Quantization functions */
void ggml_compute_forward_mul_mat_q4_0_f32(
    const struct ggml_tensor *src0,
    const struct ggml_tensor *src1,
    struct ggml_tensor *dst);

/* Debug */
void ggml_print_tensor_info(const struct ggml_tensor *t);

#endif /* _LLAMUX_GGML_KERNEL_H */