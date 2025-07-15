/*
 * Memory Reservation for Llamux
 * 
 * Handles boot-time memory reservation for LLM models.
 * This ensures we have contiguous physical memory for model weights.
 */

#ifndef _LLAMUX_MEMORY_RESERVE_H
#define _LLAMUX_MEMORY_RESERVE_H

#include <linux/types.h>
#include <linux/memblock.h>

/* Default reservation size - 2GB for TinyLlama with headroom */
#define LLAMUX_DEFAULT_RESERVE_SIZE (2ULL * 1024 * 1024 * 1024)

/* Memory region for LLM */
struct llamux_memory_region {
    phys_addr_t phys_addr;    /* Physical address */
    void *virt_addr;          /* Virtual address (mapped) */
    size_t size;              /* Size in bytes */
    bool reserved;            /* Successfully reserved? */
    bool mapped;              /* Successfully mapped? */
};

/* Global memory region */
extern struct llamux_memory_region llamux_mem_region;

/* Function prototypes */
int llamux_reserve_memory(void);
int llamux_map_reserved_memory(void);
void llamux_unmap_reserved_memory(void);
void *llamux_alloc_from_reserved(size_t size);
void llamux_free_to_reserved(void *ptr, size_t size);

/* Boot parameter parsing */
int __init llamux_parse_mem_size(char *str);

/* Memory info */
void llamux_print_memory_info(void);

#endif /* _LLAMUX_MEMORY_RESERVE_H */