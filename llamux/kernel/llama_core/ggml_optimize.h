#ifndef GGML_OPTIMIZE_H
#define GGML_OPTIMIZE_H

#include <linux/kernel.h>
#include <asm/fpu/api.h>

// Enable kernel SIMD operations
static inline void kernel_simd_begin(void) {
    kernel_fpu_begin();
}

static inline void kernel_simd_end(void) {
    kernel_fpu_end();
}

// Optimized dot product for float arrays
static inline float dot_product_f32(const float* a, const float* b, int n) {
    float sum = 0.0f;
    int i;
    
    // Unroll loop for better performance
    for (i = 0; i < n - 3; i += 4) {
        sum += a[i] * b[i];
        sum += a[i+1] * b[i+1];
        sum += a[i+2] * b[i+2];
        sum += a[i+3] * b[i+3];
    }
    
    // Handle remaining elements
    for (; i < n; i++) {
        sum += a[i] * b[i];
    }
    
    return sum;
}

// Optimized matrix multiplication for small blocks
static inline void matmul_block_f32(
    const float* A, const float* B, float* C,
    int M, int N, int K,
    int block_size
) {
    int i, j, k, ii, jj, kk;
    
    // Block-wise computation for better cache usage
    for (ii = 0; ii < M; ii += block_size) {
        for (jj = 0; jj < N; jj += block_size) {
            for (kk = 0; kk < K; kk += block_size) {
                // Compute block
                for (i = ii; i < min(ii + block_size, M); i++) {
                    for (j = jj; j < min(jj + block_size, N); j++) {
                        float sum = C[i * N + j];
                        for (k = kk; k < min(kk + block_size, K); k++) {
                            sum += A[i * K + k] * B[k * N + j];
                        }
                        C[i * N + j] = sum;
                    }
                }
            }
        }
    }
}

// Integer-only operations for Q4_K
static inline int32_t dot_product_q4k_int(
    const uint8_t* x, const uint8_t* y,
    int32_t scale_x, int32_t scale_y,
    int n
) {
    int32_t sum = 0;
    int i;
    
    for (i = 0; i < n; i++) {
        // Extract 4-bit values
        uint8_t vx = (i & 1) ? (x[i/2] >> 4) : (x[i/2] & 0xF);
        uint8_t vy = (i & 1) ? (y[i/2] >> 4) : (y[i/2] & 0xF);
        
        // Integer multiplication
        sum += (int32_t)(vx - 8) * (int32_t)(vy - 8);
    }
    
    // Scale result (approximate)
    return (sum * scale_x * scale_y) >> 16;
}

#endif /* GGML_OPTIMIZE_H */