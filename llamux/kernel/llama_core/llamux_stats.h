/*
 * Performance Statistics for Llamux
 */

#ifndef _LLAMUX_STATS_H
#define _LLAMUX_STATS_H

#include <linux/atomic.h>

/* Performance statistics */
struct llamux_stats {
    /* Token generation stats */
    atomic64_t total_tokens_generated;
    atomic64_t total_inference_time_ms;
    atomic_t current_tokens_per_sec;
    atomic64_t last_inference_start;
    atomic_t last_batch_tokens;
    
    /* Cache stats */
    atomic64_t cache_hits;
    atomic64_t cache_misses;
    
    /* Request stats */
    atomic64_t total_requests;
    atomic64_t failed_requests;
    
    /* Memory stats */
    atomic64_t peak_memory_used;
};

/* Global performance stats */
extern struct llamux_stats llamux_perf_stats;

#endif /* _LLAMUX_STATS_H */