/*
 * Weight Cache Implementation for Llamux
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include "weight_cache.h"
#include "quantize.h"
#include "llamux_stats.h"

/* Initialize weight cache */
int llama_weight_cache_init(struct llama_weight_cache *cache, int n_layers, size_t max_size) {
    if (!cache || n_layers <= 0 || n_layers > 128) {
        return -EINVAL;
    }
    
    memset(cache, 0, sizeof(*cache));
    
    cache->n_layers = n_layers;
    cache->max_cache_size = max_size;
    cache->enabled = true;
    
    mutex_init(&cache->cache_lock);
    atomic_set(&cache->cache_hits, 0);
    atomic_set(&cache->cache_misses, 0);
    
    pr_info("🦙 Weight Cache: Initialized for %d layers, max size %zu MB\n", 
            n_layers, max_size / (1024 * 1024));
    pr_info("🦙 Weight Cache: Ready to accelerate inference!\n");
    
    return 0;
}

/* Free weight cache */
void llama_weight_cache_free(struct llama_weight_cache *cache) {
    int i, j;
    
    if (!cache) return;
    
    mutex_lock(&cache->cache_lock);
    
    /* Free all cached weights */
    for (i = 0; i < cache->n_layers; i++) {
        for (j = 0; j < MAX_WEIGHT_TYPES; j++) {
            struct weight_cache_entry *entry = &cache->weights[i][j];
            if (entry->cached && entry->dequantized) {
                kvfree(entry->dequantized);
                entry->dequantized = NULL;
                entry->cached = false;
            }
        }
    }
    
    cache->total_cache_size = 0;
    cache->enabled = false;
    
    mutex_unlock(&cache->cache_lock);
    
    pr_info("🦙 Weight Cache: Freed all cached weights\n");
}

/* Get cached weight or dequantize on demand */
float *llama_weight_cache_get(struct llama_weight_cache *cache, 
                             int layer, 
                             enum weight_type type,
                             const void *quantized_data,
                             size_t n_elements,
                             enum ggml_type quant_type) {
    struct weight_cache_entry *entry;
    float *result = NULL;
    
    if (!cache || !cache->enabled || layer >= cache->n_layers || 
        type >= MAX_WEIGHT_TYPES || !quantized_data) {
        return NULL;
    }
    
    entry = &cache->weights[layer][type];
    
    /* Fast path - already cached */
    if (entry->cached && entry->dequantized) {
        atomic_inc(&cache->cache_hits);
        /* Update global stats */
        extern struct llamux_stats llamux_perf_stats;
        atomic64_inc(&llamux_perf_stats.cache_hits);
        
        atomic_inc(&entry->ref_count);
        entry->last_access = get_jiffies_64();
        return entry->dequantized;
    }
    
    /* Slow path - need to dequantize */
    mutex_lock(&cache->cache_lock);
    
    /* Double-check under lock */
    if (entry->cached && entry->dequantized) {
        atomic_inc(&cache->cache_hits);
        /* Update global stats */
        extern struct llamux_stats llamux_perf_stats;
        atomic64_inc(&llamux_perf_stats.cache_hits);
        
        atomic_inc(&entry->ref_count);
        entry->last_access = get_jiffies_64();
        mutex_unlock(&cache->cache_lock);
        return entry->dequantized;
    }
    
    atomic_inc(&cache->cache_misses);
    /* Update global stats */
    extern struct llamux_stats llamux_perf_stats;
    atomic64_inc(&llamux_perf_stats.cache_misses);
    
    /* Calculate size needed */
    size_t size = n_elements * sizeof(float);
    
    /* Check cache size limit */
    if (cache->total_cache_size + size > cache->max_cache_size) {
        pr_warn("🦙 Weight Cache: Would exceed limit (%zu + %zu > %zu)\n",
                cache->total_cache_size, size, cache->max_cache_size);
        mutex_unlock(&cache->cache_lock);
        return NULL;
    }
    
    /* Allocate buffer for dequantized weights */
    entry->dequantized = kvmalloc(size, GFP_KERNEL);
    if (!entry->dequantized) {
        pr_err("🦙 Weight Cache: Failed to allocate %zu bytes\n", size);
        mutex_unlock(&cache->cache_lock);
        return NULL;
    }
    
    /* Dequantize the weights */
    pr_info("🦙 Weight Cache: Dequantizing layer %d type %d (%zu elements)\n",
            layer, type, n_elements);
    
    dequantize_row(quantized_data, entry->dequantized, n_elements, quant_type);
    
    /* Update cache entry */
    entry->quantized = (void *)quantized_data;
    entry->size = size;
    entry->n_elements = n_elements;
    entry->type = quant_type;
    entry->cached = true;
    entry->last_access = get_jiffies_64();
    atomic_set(&entry->ref_count, 1);
    
    /* Update global stats */
    cache->total_cache_size += size;
    
    mutex_unlock(&cache->cache_lock);
    
    return entry->dequantized;
}

/* Release weight reference */
void llama_weight_cache_release(struct llama_weight_cache *cache,
                               int layer,
                               enum weight_type type) {
    struct weight_cache_entry *entry;
    
    if (!cache || !cache->enabled || layer >= cache->n_layers || 
        type >= MAX_WEIGHT_TYPES) {
        return;
    }
    
    entry = &cache->weights[layer][type];
    
    if (entry->cached) {
        atomic_dec(&entry->ref_count);
    }
}

/* Cache statistics */
void llama_weight_cache_stats(struct llama_weight_cache *cache) {
    if (!cache) return;
    
    int hits = atomic_read(&cache->cache_hits);
    int misses = atomic_read(&cache->cache_misses);
    int total = hits + misses;
    
    pr_info("🦙 Weight Cache Stats:\n");
    pr_info("  Total size: %zu MB / %zu MB\n", 
            cache->total_cache_size / (1024 * 1024),
            cache->max_cache_size / (1024 * 1024));
    pr_info("  Hit rate: %d%% (%d hits, %d misses)\n",
            total > 0 ? (hits * 100) / total : 0,
            hits, misses);
}