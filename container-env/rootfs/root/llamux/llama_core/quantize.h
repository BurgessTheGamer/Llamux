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
            uint8_t scales[K_SCALE_SIZE];   /* 12 bytes of scales */
            uint8_t qs[QK_K/2];            /* 128 bytes of 4-bit quants */
            int16_t d;                      /* super-block scale */
            int16_t dmin;                   /* super-block min */
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

#endif /* _LLAMUX_QUANTIZE_H */