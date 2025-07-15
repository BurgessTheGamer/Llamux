/*
 * Simplified Memory Management for Llamux
 * 
 * Uses vmalloc for large allocations instead of boot-time reservation.
 * This is a temporary solution for development and testing.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "memory_reserve.h"

/* Global memory region for Llamux */
struct llamux_memory_region llamux_mem_region = {
    .phys_addr = 0,
    .virt_addr = NULL,
    .size = 0,
    .reserved = false,
    .mapped = false
};

/* Track allocation offset separately */
static size_t allocation_offset = 0;

static DEFINE_MUTEX(allocation_lock);

/* Reserve memory (simplified - just track size) */
int llamux_reserve_memory(void)
{
    if (llamux_mem_region.reserved) {
        pr_warn("🦙 Llamux: Memory already reserved\n");
        return 0;
    }
    
    /* Use default size of 2GB */
    size_t size = LLAMUX_DEFAULT_RESERVE_SIZE;
    
    /* Round up to page size */
    size = PAGE_ALIGN(size);
    
    /* Store the reservation info */
    llamux_mem_region.size = size;
    llamux_mem_region.reserved = true;
    
    pr_info("🦙 Llamux: Memory reservation prepared for %zu MB\n",
            size / (1024 * 1024));
    
    return 0;
}

/* Map memory using vmalloc */
int llamux_map_reserved_memory(void)
{
    if (!llamux_mem_region.reserved || !llamux_mem_region.size) {
        pr_err("🦙 Llamux: No memory reserved to map\n");
        return -EINVAL;
    }
    
    if (llamux_mem_region.mapped) {
        pr_warn("🦙 Llamux: Memory already mapped\n");
        return 0;
    }
    
    /* Use vmalloc for large allocations */
    llamux_mem_region.virt_addr = vzalloc(llamux_mem_region.size);
    if (!llamux_mem_region.virt_addr) {
        pr_err("🦙 Llamux: Failed to allocate %zu MB\n",
               llamux_mem_region.size / (1024 * 1024));
        return -ENOMEM;
    }
    
    llamux_mem_region.mapped = true;
    allocation_offset = 0;
    
    pr_info("🦙 Llamux: Allocated %zu MB at virtual address %p\n",
            llamux_mem_region.size / (1024 * 1024),
            llamux_mem_region.virt_addr);
    
    return 0;
}

/* Unmap memory */
void llamux_unmap_reserved_memory(void)
{
    if (!llamux_mem_region.mapped || !llamux_mem_region.virt_addr) {
        return;
    }
    
    vfree(llamux_mem_region.virt_addr);
    llamux_mem_region.virt_addr = NULL;
    llamux_mem_region.mapped = false;
    allocation_offset = 0;
    
    pr_info("🦙 Llamux: Freed reserved memory\n");
}

/* Allocate from reserved pool */
void *llamux_alloc_from_reserved(size_t size)
{
    void *ptr = NULL;
    size_t aligned_size;
    
    if (!llamux_mem_region.mapped || !llamux_mem_region.virt_addr) {
        pr_err("🦙 Llamux: Reserved memory not mapped\n");
        return NULL;
    }
    
    /* Align size to 64 bytes for cache efficiency */
    aligned_size = ALIGN(size, 64);
    
    mutex_lock(&allocation_lock);
    
    if (allocation_offset + aligned_size > llamux_mem_region.size) {
        pr_err("🦙 Llamux: Out of reserved memory (requested %zu, have %zu)\n",
               aligned_size, llamux_mem_region.size - allocation_offset);
        goto out;
    }
    
    ptr = llamux_mem_region.virt_addr + allocation_offset;
    allocation_offset += aligned_size;
    
    pr_debug("🦙 Llamux: Allocated %zu bytes from reserved memory at offset %zu\n",
             aligned_size, allocation_offset - aligned_size);
    
out:
    mutex_unlock(&allocation_lock);
    return ptr;
}

/* Free is a no-op with bump allocator */
void llamux_free_to_reserved(void *ptr, size_t size)
{
    /* TODO: Implement proper memory management if needed */
    pr_debug("🦙 Llamux: Free to reserved called (no-op)\n");
}

/* Print memory info */
void llamux_print_memory_info(void)
{
    size_t used = allocation_offset;
    size_t total = llamux_mem_region.size;
    
    pr_info("🦙 Llamux Memory Status:\n");
    pr_info("  Total: %zu MB\n", total / (1024 * 1024));
    pr_info("  Used: %zu MB (%u%%)\n", used / (1024 * 1024),
            total > 0 ? (unsigned int)(used * 100 / total) : 0);
    pr_info("  Free: %zu MB\n", (total - used) / (1024 * 1024));
    pr_info("  Virtual Address: %p\n", llamux_mem_region.virt_addr);
}


/* Kernel command line parsing stub */
int __init llamux_parse_mem_size(char *str)
{
    /* This would parse boot parameters in a real implementation */
    return 0;
}
__setup("llamux_mem=", llamux_parse_mem_size);