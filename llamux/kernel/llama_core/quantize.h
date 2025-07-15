/*
 * Quantization support for Llamux
 * 
 * Implements dequantization for Q4_K and other formats
 */

#ifndef _LLAMUX_QUANTIZE_H
#define _LLAMUX_QUANTIZE_H

#include <linux/types.h>
#include "gguf_parser.h"

/* Q4_K block size */
#define QK_K 256
#define K_SCALE_SIZE 12

/* Q4_K block structure */
struct block_q4_K {
    union {
        struct {
            uint16_t d;                     /* super-block scale (FP16) */
            uint16_t dmin;                  /* super-block min (FP16) */
            uint8_t scales[K_SCALE_SIZE];   /* 12 bytes of scales */
            uint8_t qs[QK_K/2];            /* 128 bytes of 4-bit quants */
        } __packed;
        uint8_t data[144];                  /* Total size for Q4_K */
    };
} __packed;

/* Dequantize Q4_K block to float */
void dequantize_q4_K(const struct block_q4_K *x, float *y, int k);

/* Dequantize Q6_K block to float */
void dequantize_q6_K(const void *x, float *y, int k);

/* Generic dequantization based on type */
void dequantize_row(const void *x, float *y, int k, enum ggml_type type);

/* FP16 to FP32 conversion */
static inline float ggml_fp16_to_fp32(uint16_t h) {
    union { uint32_t u; float f; } o;
    
    uint32_t sign = (h >> 15) & 1;
    uint32_t exp = (h >> 10) & 0x1f;
    uint32_t mant = h & 0x3ff;
    
    if (exp == 0) {
        /* Zero or subnormal */
        if (mant == 0) {
            /* Zero */
            o.u = sign << 31;
        } else {
            /* Subnormal - convert to normalized */
            exp = 1;
            while ((mant & 0x400) == 0) {
                mant <<= 1;
                exp--;
            }
            mant &= 0x3ff;
            o.u = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        /* Inf or NaN */
        if (mant == 0) {
            /* Infinity - use large value in kernel */
            o.u = (sign << 31) | (0xfe << 23);
        } else {
            /* NaN - return 0 in kernel */
            o.f = 0.0f;
            return o.f;
        }
    } else {
        /* Normal number */
        o.u = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
    }
    
    return o.f;
}

#endif /* _LLAMUX_QUANTIZE_H */