/*
 * Quantization implementation for Llamux
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <asm/fpu/api.h>
#include "quantize.h"
#include "gguf_parser.h"

/* Dequantize Q4_K block - kernel-friendly implementation */
void dequantize_q4_K(const struct block_q4_K *x, float *y, int k) {
    const int nb = k / QK_K;
    static int debug_count = 0;
    
    /* Reset debug count for testing */
    debug_count = 0;
    
    for (int i = 0; i < nb; i++) {
        const struct block_q4_K *block = &x[i];
        
        /* Be a good citizen - yield CPU if needed (but less often) */
        if (i > 0 && (i % 256) == 0) {
            if (need_resched()) {
                cond_resched();
            }
        }
        
        /* Extract scales - Q4_K uses complex scale packing */
        /* CRITICAL: d and dmin are FP16 (half precision), not regular floats! */
        kernel_fpu_begin();
        const float d = ggml_fp16_to_fp32(block->d);
        const float dmin = ggml_fp16_to_fp32(block->dmin);
        kernel_fpu_end();
        
        /* Unpack scales - Q4_K has 2 scales per byte for 16 groups */
        uint8_t scales[16];
        for (int j = 0; j < 8; j++) {
            uint8_t byte = block->scales[j];
            scales[j*2] = byte & 63;
            scales[j*2 + 1] = byte >> 6;
        }
        /* Remaining 4 bytes contain mins for each scale */
        uint8_t mins[16];
        for (int j = 0; j < 4; j++) {
            uint8_t byte = block->scales[8 + j];
            mins[j*4] = (byte & 3);
            mins[j*4 + 1] = ((byte >> 2) & 3);
            mins[j*4 + 2] = ((byte >> 4) & 3);
            mins[j*4 + 3] = (byte >> 6);
        }
        
        /* Debug first few blocks */
        if (debug_count < 3 && i < 3) {
            kernel_fpu_begin();
            int d_int = (int)(d * 1000000);
            int dmin_int = (int)(dmin * 1000000);
            kernel_fpu_end();
            pr_info("Q4_K dequant block[%d]: d_fp16=0x%04x->%d/1M, dmin_fp16=0x%04x->%d/1M\n",
                    i, block->d, d_int, block->dmin, dmin_int);
            pr_info("  First 4 scales: %d %d %d %d (raw bytes: 0x%02x 0x%02x 0x%02x)\n",
                    scales[0], scales[1], scales[2], scales[3],
                    block->scales[0], block->scales[1], block->scales[2]);
            pr_info("  First 4 qs bytes: 0x%02x 0x%02x 0x%02x 0x%02x\n",
                    block->qs[0], block->qs[1], block->qs[2], block->qs[3]);
            if (i == 0) debug_count++;
        }
        
        /* Dequantize values - 16 groups of 16 values each */
        float *dst = y + i*QK_K;
        kernel_fpu_begin();
        
        for (int j = 0; j < 16; j++) {
            /* Q4_K formula: y = d * (x - 8) * sc + dmin * m */
            const float sc = (float)scales[j];
            const float m = (float)(mins[j] + 1);
            
            const uint8_t *q = &block->qs[j * 8];
            float *out = &dst[j * 16];
            
            /* Dequantize 16 values */
            for (int l = 0; l < 8; l++) {
                const uint8_t vi = q[l];
                const int8_t vi0 = (vi & 0xF);
                const int8_t vi1 = (vi >> 4);
                
                out[l*2 + 0] = d * (vi0 - 8.0f) * sc + dmin * m;
                out[l*2 + 1] = d * (vi1 - 8.0f) * sc + dmin * m;
            }
        }
        
        kernel_fpu_end();
        
        /* Debug: check first few output values */
        if (debug_count <= 3 && i == 0) {
            kernel_fpu_begin();
            int v0 = (int)(dst[0] * 1000);
            int v1 = (int)(dst[1] * 1000);
            int v2 = (int)(dst[2] * 1000);
            int v3 = (int)(dst[3] * 1000);
            kernel_fpu_end();
            pr_info("  First 4 dequantized values (x1000): %d %d %d %d\n",
                    v0, v1, v2, v3);
        }
    }
}

/* Dequantize Q6_K block */
void dequantize_q6_K(const void *x, float *y, int k) {
    /* Simplified implementation */
    /* Q6_K uses 6-bit quantization with more complex scaling */
    const int nb = k / QK_K;
    const uint8_t *data = (const uint8_t *)x;
    
    for (int i = 0; i < nb; i++) {
        /* Each Q6_K block is 210 bytes */
        const float scale = 1.0f; /* Placeholder scale */
        
        for (int j = 0; j < QK_K; j++) {
            /* Simplified - just use a dummy value for now */
            y[i*QK_K + j] = scale * (float)j / QK_K;
        }
        
        data += 210; /* Q6_K block size */
    }
}

/* Generic dequantization */
void dequantize_row(const void *x, float *y, int k, enum ggml_type type) {
    switch (type) {
    case GGML_TYPE_F32:
        /* Already float, just copy */
        memcpy(y, x, k * sizeof(float));
        break;
        
    case GGML_TYPE_Q4_K:
        dequantize_q4_K((const struct block_q4_K *)x, y, k);
        break;
        
    case GGML_TYPE_Q6_K:
        dequantize_q6_K(x, y, k);
        break;
        
    default:
        pr_warn("🦙 Llamux: Unsupported quantization type %d\n", type);
        /* Fill with zeros */
        memset(y, 0, k * sizeof(float));
        break;
    }
}