#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

// This module will help us inspect actual tensor data from the loaded model

static void inspect_memory_as_hex(const char *name, void *data, size_t bytes) {
    int i;
    uint8_t *ptr = (uint8_t *)data;
    
    pr_info("=== Memory dump of %s (first %zu bytes) ===\n", name, bytes);
    
    for (i = 0; i < bytes && i < 64; i++) {
        if (i % 16 == 0) {
            if (i > 0) pr_cont("\n");
            pr_info("%04x: ", i);
        }
        pr_cont("%02x ", ptr[i]);
    }
    pr_cont("\n");
}

static void inspect_memory_as_float(const char *name, void *data, size_t count) {
    int i;
    float *ptr = (float *)data;
    
    pr_info("=== Float interpretation of %s (first %zu values) ===\n", name, count);
    
    for (i = 0; i < count && i < 8; i++) {
        pr_info("  [%d] = %f (hex: 0x%08x)\n", i, ptr[i], *(uint32_t*)&ptr[i]);
    }
}

static void analyze_q4k_block(const char *name, void *data) {
    struct {
        uint16_t d;
        uint16_t dmin;
        uint8_t scales[12];
        uint8_t qs[128];
    } *block = data;
    
    pr_info("=== Q4_K Block Analysis: %s ===\n", name);
    pr_info("  d = 0x%04x\n", block->d);
    pr_info("  dmin = 0x%04x\n", block->dmin);
    pr_info("  First 4 scales: %02x %02x %02x %02x\n",
            block->scales[0], block->scales[1], block->scales[2], block->scales[3]);
    pr_info("  First 8 qs: %02x %02x %02x %02x %02x %02x %02x %02x\n",
            block->qs[0], block->qs[1], block->qs[2], block->qs[3],
            block->qs[4], block->qs[5], block->qs[6], block->qs[7]);
}

// Export these functions so llamux can call them
void llamux_inspect_tensor(const char *name, void *data, size_t bytes, int type) {
    pr_info("\n========== TENSOR INSPECTION: %s ==========\n", name);
    pr_info("Size: %zu bytes, Type: %d\n", bytes, type);
    
    inspect_memory_as_hex(name, data, bytes);
    
    if (type == 0) { // F32
        inspect_memory_as_float(name, data, bytes / sizeof(float));
    } else if (type == 2) { // Q4_K
        analyze_q4k_block(name, data);
    }
}
EXPORT_SYMBOL(llamux_inspect_tensor);

static int __init inspect_init(void) {
    pr_info("Tensor Inspection Module Loaded\n");
    return 0;
}

static void __exit inspect_exit(void) {
    pr_info("Tensor Inspection Module Unloaded\n");
}

module_init(inspect_init);
module_exit(inspect_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tensor Inspection Helper");