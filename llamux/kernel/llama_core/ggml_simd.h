/*
 * SIMD optimizations for GGML operations
 */

#ifndef _LLAMUX_GGML_SIMD_H
#define _LLAMUX_GGML_SIMD_H

#include <linux/kernel.h>

/* Disable AVX2 in kernel space - no immintrin.h available */
#ifdef __KERNEL__
#undef __AVX2__
#endif

#ifdef __AVX2__
#include <immintrin.h>

/* AVX2 optimized dot product */
static inline float ggml_vec_dot_f32_avx2(const float *x, const float *y, int n) {
    __m256 sum = _mm256_setzero_ps();
    int i;
    
    /* Process 8 floats at a time */
    for (i = 0; i <= n - 8; i += 8) {
        __m256 vx = _mm256_loadu_ps(x + i);
        __m256 vy = _mm256_loadu_ps(y + i);
        sum = _mm256_fmadd_ps(vx, vy, sum);
    }
    
    /* Horizontal sum */
    __m128 sum_high = _mm256_extractf128_ps(sum, 1);
    __m128 sum_low = _mm256_castps256_ps128(sum);
    __m128 sum_128 = _mm_add_ps(sum_low, sum_high);
    
    sum_128 = _mm_hadd_ps(sum_128, sum_128);
    sum_128 = _mm_hadd_ps(sum_128, sum_128);
    
    float result = _mm_cvtss_f32(sum_128);
    
    /* Handle remaining elements */
    for (; i < n; i++) {
        result += x[i] * y[i];
    }
    
    return result;
}

#else

/* Optimized scalar implementation with loop unrolling */
static inline float ggml_vec_dot_f32_scalar(const float *x, const float *y, int n) {
    float sum0 = 0.0f, sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;
    int i;
    
    /* Unroll by 8 for better performance */
    for (i = 0; i < n - 7; i += 8) {
        sum0 += x[i] * y[i];
        sum1 += x[i+1] * y[i+1];
        sum2 += x[i+2] * y[i+2];
        sum3 += x[i+3] * y[i+3];
        sum0 += x[i+4] * y[i+4];
        sum1 += x[i+5] * y[i+5];
        sum2 += x[i+6] * y[i+6];
        sum3 += x[i+7] * y[i+7];
    }
    
    /* Handle remaining elements */
    for (; i < n; i++) {
        sum0 += x[i] * y[i];
    }
    
    return sum0 + sum1 + sum2 + sum3;
}

#define ggml_vec_dot_f32_avx2 ggml_vec_dot_f32_scalar

#endif /* __AVX2__ */

/* Generic dot product that selects best implementation */
static inline float ggml_vec_dot_f32(const float *x, const float *y, int n) {
    return ggml_vec_dot_f32_avx2(x, y, n);
}

#endif /* _LLAMUX_GGML_SIMD_H */