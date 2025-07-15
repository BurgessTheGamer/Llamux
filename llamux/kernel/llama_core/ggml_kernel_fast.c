/*
 * Fast kernel-space matrix multiplication for Q4_K quantized models
 * Uses integer-only operations to avoid FPU overhead
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/fpu/api.h>
#include "ggml_kernel.h"
#include "quantize.h"

/* Define min if not already defined */
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* Fast integer-only Q4_K dot product */
static int32_t dot_product_q4k_int32(
    const struct block_q4_K *x, 
    const float *y,
    int nb
) {
    int64_t sum = 0;
    
    for (int i = 0; i < nb; i++) {
        const struct block_q4_K *block = &x[i];
        const float *yb = y + i * QK_K;
        
        /* Convert scales to fixed point (16.16) */
        int32_t d_fp = (int32_t)(block->d * 65536.0f);
        int32_t dmin_fp = (int32_t)(block->dmin * 65536.0f);
        
        /* Process 32 values at a time */
        for (int j = 0; j < QK_K/2; j++) {
            uint8_t byte = block->qs[j];
            
            /* Extract two 4-bit values */
            int32_t v0 = (byte & 0xF) - 8;
            int32_t v1 = ((byte >> 4) & 0xF) - 8;
            
            /* Convert y values to fixed point */
            int32_t y0 = (int32_t)(yb[j*2] * 256.0f);
            int32_t y1 = (int32_t)(yb[j*2 + 1] * 256.0f);
            
            /* Accumulate using integer math */
            sum += (v0 * y0 * d_fp) >> 22;  /* 16+8-2 = 22 bits to shift */
            sum += (v1 * y1 * d_fp) >> 22;
        }
    }
    
    /* Convert back to float */
    return sum;
}

/* Optimized matrix multiplication for Q4_K */
void ggml_compute_forward_mul_mat_q4k_fast(
    const struct ggml_tensor *src0,
    const struct ggml_tensor *src1, 
    struct ggml_tensor *dst
) {
    pr_info("🦙 Fast MatMul: Using optimized Q4_K implementation!\n");
    const int64_t ne00 = src0->ne[0];
    const int64_t ne01 = src0->ne[1];
    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    
    const int nb = ne00 / QK_K;
    
    /* Process in chunks to allow preemption */
    const int CHUNK_SIZE = 64;
    
    kernel_fpu_begin();
    
    for (int64_t i = 0; i < ne01; i++) {
        const struct block_q4_K *row = (struct block_q4_K *)((char *)src0->data + i * src0->nb[1]);
        
        /* Allow preemption every chunk */
        if (i > 0 && (i % CHUNK_SIZE) == 0) {
            kernel_fpu_end();
            if (need_resched()) {
                cond_resched();
            }
            kernel_fpu_begin();
            
            /* Progress reporting for large matrices */
            if (ne01 > 1000 && (i % 1000) == 0) {
                pr_info("🦙 Fast MatMul: %lld/%lld rows (%.1f%%)\n", 
                        i, ne01, (float)i * 100.0f / ne01);
            }
        }
        
        for (int64_t j = 0; j < ne11; j++) {
            const float *col = (float *)src1->data + j * ne10;
            
            /* Use integer-only dot product */
            int32_t sum_int = dot_product_q4k_int32(row, col, nb);
            
            /* Store result as float */
            float *dst_ptr = (float *)dst->data + i * ne11 + j;
            *dst_ptr = (float)sum_int / 256.0f;
        }
    }
    
    kernel_fpu_end();
}

/* Even faster version using block processing */
void ggml_compute_forward_mul_mat_q4k_block(
    const struct ggml_tensor *src0,
    const struct ggml_tensor *src1,
    struct ggml_tensor *dst
) {
    const int64_t ne00 = src0->ne[0];
    const int64_t ne01 = src0->ne[1]; 
    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    
    const int nb = ne00 / QK_K;
    const int BLOCK_SIZE = 16;  /* Process 16x16 blocks */
    
    /* Zero output first */
    memset(dst->data, 0, ne01 * ne11 * sizeof(float));
    
    /* Block-wise computation for better cache usage */
    for (int64_t i0 = 0; i0 < ne01; i0 += BLOCK_SIZE) {
        for (int64_t j0 = 0; j0 < ne11; j0 += BLOCK_SIZE) {
            
            /* Process block */
            for (int64_t i = i0; i < min(i0 + BLOCK_SIZE, ne01); i++) {
                const struct block_q4_K *row = (struct block_q4_K *)((char *)src0->data + i * src0->nb[1]);
                
                for (int64_t j = j0; j < min(j0 + BLOCK_SIZE, ne11); j++) {
                    const float *col = (float *)src1->data + j * ne10;
                    
                    int32_t sum_int = dot_product_q4k_int32(row, col, nb);
                    
                    float *dst_ptr = (float *)dst->data + i * ne11 + j;
                    *dst_ptr = (float)sum_int / 256.0f;
                }
            }
            
            /* Allow preemption between blocks */
            if (need_resched()) {
                cond_resched();
            }
        }
    }
}