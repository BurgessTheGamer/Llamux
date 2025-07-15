/*
 * Memory Reservation Implementation for Llamux
 * 
 * Manages a large contiguous memory region for LLM model storage.
 * Memory is reserved at boot time via kernel command line.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <asm/page.h>
#include "memory_reserve.h"

/* Global memory region for Llamux */
struct llamux_memory_region llamux_mem_region = {
    .phys_addr = 0,
    .virt_addr = NULL,
    .size = LLAMUX_DEFAULT_RESERVE_SIZE,
    .reserved = false,
    .mapped = false
};

/* Size to reserve - can be overridden by boot parameter */
static size_t llamux_reserve_size = LLAMUX_DEFAULT_RESERVE_SIZE;

/* Current allocation offset within reserved region */
static size_t allocation_offset = 0;
static DEFINE_MUTEX(allocation_lock);

/*
 * Parse boot parameter: llamux_mem=size
 * Example: llamux_mem=2G or llamux_mem=2048M
 */
int __init llamux_parse_mem_size(char *str)
{
    char *endp;
    unsigned long size;
    
    if (!str)
        return -EINVAL;
    
    size = memparse(str, &endp);
    
    if (size < (512 * 1024 * 1024)) {
        pr_err("🦙 Llamux: Minimum memory reservation is 512MB\n");
        return -EINVAL;
    }
    
    if (size > (4ULL * 1024 * 1024 * 1024)) {
        pr_err("🦙 Llamux: Maximum memory reservation is 4GB\n");
        return -EINVAL;
    }
    
    llamux_reserve_size = size;
    llamux_mem_region.size = size;
    
    pr_info("🦙 Llamux: Memory reservation set to %zu MB\n", 
            size / (1024 * 1024));
    
    return 0;
}
__setup("llamux_mem=", llamux_parse_mem_size);

/*
 * Reserve memory at boot time
 * This should be called very early in boot process
 */
int __init llamux_reserve_memory(void)
{
    phys_addr_t phys;
    
    /* Try to reserve memory */
    phys = memblock_find_in_range(0, MEMBLOCK_ALLOC_ACCESSIBLE,
                                  llamux_reserve_size, PAGE_SIZE);
    
    if (!phys) {
        pr_err("🦙 Llamux: Failed to find %zu MB of contiguous memory\n",
               llamux_reserve_size / (1024 * 1024));
        return -ENOMEM;
    }
    
    /* Reserve the memory */
    if (memblock_reserve(phys, llamux_reserve_size)) {
        pr_err("🦙 Llamux: Failed to reserve memory at 0x%llx\n", phys);
        return -ENOMEM;
    }
    
    llamux_mem_region.phys_addr = phys;
    llamux_mem_region.reserved = true;
    
    pr_info("🦙 Llamux: Reserved %zu MB at physical address 0x%llx\n",
            llamux_reserve_size / (1024 * 1024), phys);
    
    return 0;
}

/*
 * Map reserved memory into kernel virtual address space
 * This is called when the module loads
 */
int llamux_map_reserved_memory(void)
{
    if (!llamux_mem_region.reserved) {
        pr_err("🦙 Llamux: No memory reserved for mapping\n");
        return -EINVAL;
    }
    
    if (llamux_mem_region.mapped) {
        pr_warn("🦙 Llamux: Memory already mapped\n");
        return 0;
    }
    
    /* Map the physical memory */
    llamux_mem_region.virt_addr = ioremap_cache(llamux_mem_region.phys_addr,
                                                llamux_mem_region.size);
    
    if (!llamux_mem_region.virt_addr) {
        pr_err("🦙 Llamux: Failed to map reserved memory\n");
        return -ENOMEM;
    }
    
    llamux_mem_region.mapped = true;
    allocation_offset = 0;
    
    pr_info("🦙 Llamux: Mapped %zu MB to virtual address %p\n",
            llamux_mem_region.size / (1024 * 1024),
            llamux_mem_region.virt_addr);
    
    /* Clear the memory */
    memset(llamux_mem_region.virt_addr, 0, llamux_mem_region.size);
    
    return 0;
}

/*
 * Unmap reserved memory
 * Called when module unloads
 */
void llamux_unmap_reserved_memory(void)
{
    if (!llamux_mem_region.mapped || !llamux_mem_region.virt_addr) {
        return;
    }
    
    iounmap(llamux_mem_region.virt_addr);
    llamux_mem_region.virt_addr = NULL;
    llamux_mem_region.mapped = false;
    allocation_offset = 0;
    
    pr_info("🦙 Llamux: Unmapped reserved memory\n");
}

/*
 * Allocate memory from the reserved region
 * Simple bump allocator - no free support for now
 */
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

/*
 * Free memory back to reserved region
 * Currently a no-op as we use bump allocation
 */
void llamux_free_to_reserved(void *ptr, size_t size)
{
    /* TODO: Implement proper memory management if needed */
    pr_debug("🦙 Llamux: Free to reserved called (no-op)\n");
}

/*
 * Print memory reservation info
 */
void llamux_print_memory_info(void)
{
    size_t used = allocation_offset;
    size_t free = llamux_mem_region.size - allocation_offset;
    
    pr_info("🦙 Llamux Memory Info:\n");
    pr_info("  Reserved: %s\n", llamux_mem_region.reserved ? "Yes" : "No");
    pr_info("  Mapped: %s\n", llamux_mem_region.mapped ? "Yes" : "No");
    pr_info("  Physical Address: 0x%llx\n", llamux_mem_region.phys_addr);
    pr_info("  Virtual Address: %p\n", llamux_mem_region.virt_addr);
    pr_info("  Total Size: %zu MB\n", llamux_mem_region.size / (1024 * 1024));
    pr_info("  Used: %zu MB (%.1f%%)\n", used / (1024 * 1024),
            (used * 100.0) / llamux_mem_region.size);
    pr_info("  Free: %zu MB\n", free / (1024 * 1024));
}

/*
 * Early boot initialization
 * This needs to be called from architecture-specific code
 */
static int __init llamux_memory_init(void)
{
    int ret;
    
    /* Only reserve if boot parameter was specified */
    if (llamux_reserve_size == 0) {
        pr_info("🦙 Llamux: No memory reservation requested\n");
        return 0;
    }
    
    ret = llamux_reserve_memory();
    if (ret) {
        pr_err("🦙 Llamux: Memory reservation failed\n");
        /* Continue anyway - module will fail to load later */
    }
    
    return 0;
}
/* Run very early in boot */
early_initcall(llamux_memory_init);