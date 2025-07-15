/*
 * Weight Cache System for Llamux
 * 
 * Caches dequantized weights to avoid repeated dequantization
 */

#ifndef _LLAMUX_WEIGHT_CACHE_H
#define _LLAMUX_WEIGHT_CACHE_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include "ggml_kernel.h"

/* Weight types in transformer */
enum weight_type {
    WEIGHT_WQ,      /* Query projection */
    WEIGHT_WK,      /* Key projection */
    WEIGHT_WV,      /* Value projection */
    WEIGHT_WO,      /* Output projection */
    WEIGHT_W1,      /* FFN gate */
    WEIGHT_W2,      /* FFN down */
    WEIGHT_W3,      /* FFN up */
    WEIGHT_NORM,    /* RMS norm */
    WEIGHT_EMBED,   /* Token embeddings */
    WEIGHT_OUTPUT,  /* Output projection */
    MAX_WEIGHT_TYPES
};

/* Cache entry for a single weight tensor */
struct weight_cache_entry {
    void *quantized;           /* Original quantized data */
    float *dequantized;        /* Cached F32 version */
    size_t size;              /* Size in bytes */
    size_t n_elements;        /* Number of elements */
    enum ggml_type type;      /* Original type (Q4_K, etc) */
    bool cached;              /* Is dequantized version ready? */
    atomic_t ref_count;       /* Reference count for concurrent access */
    u64 last_access;          /* For LRU eviction if needed */
};

/* Global weight cache */
struct llama_weight_cache {
    /* Cache entries per layer per weight type */
    struct weight_cache_entry weights[128][MAX_WEIGHT_TYPES];  /* Support up to 128 layers */
    
    /* Global cache stats */
    size_t total_cache_size;
    size_t max_cache_size;
    atomic_t cache_hits;
    atomic_t cache_misses;
    
    /* Synchronization */
    struct mutex cache_lock;
    
    /* Configuration */
    bool enabled;
    int n_layers;
};

/* Initialize weight cache */
int llama_weight_cache_init(struct llama_weight_cache *cache, int n_layers, size_t max_size);

/* Free weight cache */
void llama_weight_cache_free(struct llama_weight_cache *cache);

/* Get cached weight or dequantize on demand */
float *llama_weight_cache_get(struct llama_weight_cache *cache, 
                             int layer, 
                             enum weight_type type,
                             const void *quantized_data,
                             size_t n_elements,
                             enum ggml_type quant_type);

/* Release weight reference */
void llama_weight_cache_release(struct llama_weight_cache *cache,
                               int layer,
                               enum weight_type type);

/* Cache statistics */
void llama_weight_cache_stats(struct llama_weight_cache *cache);

#endif /* _LLAMUX_WEIGHT_CACHE_H */