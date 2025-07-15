#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/fpu/api.h>

// Simplified Q4_K structure for testing
typedef struct {
    uint16_t d;      // delta scale (half precision)
    uint16_t dmin;   // minimum value scale (half precision)
    uint8_t scales[12]; // 6-bit scales
    uint8_t qs[128];    // 4-bit quantized values
} test_block_q4_K;

// Half to float conversion
static float fp16_to_fp32(uint16_t h) {
    uint32_t sign = (h >> 15) & 0x1;
    uint32_t exponent = (h >> 10) & 0x1f;
    uint32_t mantissa = h & 0x3ff;
    
    if (exponent == 0) {
        // Subnormal
        return (sign ? -1.0f : 1.0f) * (mantissa / 1024.0f) * (1.0f / 16384.0f);
    } else if (exponent == 31) {
        // Inf/NaN
        return mantissa ? 0.0f : (sign ? -INFINITY : INFINITY);
    }
    
    // Normal
    float value = (1.0f + mantissa / 1024.0f) * powf(2.0f, exponent - 15);
    return sign ? -value : value;
}

static void test_known_dequantization(void) {
    test_block_q4_K test_block;
    float output[256];
    int i, j;
    
    pr_info("=== Testing Q4_K Dequantization with Known Values ===\n");
    
    // Test Case 1: Simple values
    // d = 1.0 (0x3C00 in fp16), dmin = 0.0 (0x0000)
    test_block.d = 0x3C00;
    test_block.dmin = 0x0000;
    
    // All scales = 1 (stored as 6-bit)
    memset(test_block.scales, 1, 12);
    
    // All quantized values = 0x0F (15 in both nibbles)
    memset(test_block.qs, 0xFF, 128);
    
    kernel_fpu_begin();
    
    float d = fp16_to_fp32(test_block.d);
    float dmin = fp16_to_fp32(test_block.dmin);
    
    pr_info("d = %f, dmin = %f\n", d, dmin);
    
    // Dequantize
    for (i = 0; i < 128; i++) {
        uint8_t q = test_block.qs[i];
        float scale = (float)test_block.scales[i / 16]; // Simplified scale access
        
        output[i*2] = d * scale * (q & 0xF) + dmin * scale;
        output[i*2 + 1] = d * scale * (q >> 4) + dmin * scale;
    }
    
    kernel_fpu_end();
    
    // Check first few values
    pr_info("First 8 dequantized values:\n");
    for (i = 0; i < 8; i++) {
        pr_info("  output[%d] = %f (expected: 15.0)\n", i, output[i]);
    }
    
    // Test Case 2: With offset
    test_block.d = 0x3C00;    // 1.0
    test_block.dmin = 0x3C00; // 1.0
    memset(test_block.scales, 2, 12);
    memset(test_block.qs, 0x50, 128); // 5 and 0 in nibbles
    
    kernel_fpu_begin();
    
    d = fp16_to_fp32(test_block.d);
    dmin = fp16_to_fp32(test_block.dmin);
    
    for (i = 0; i < 4; i++) {
        uint8_t q = test_block.qs[i];
        float scale = 2.0f;
        
        output[i*2] = d * scale * (q & 0xF) + dmin * scale;
        output[i*2 + 1] = d * scale * (q >> 4) + dmin * scale;
    }
    
    kernel_fpu_end();
    
    pr_info("\nTest Case 2 - With offset:\n");
    pr_info("  output[0] = %f (expected: 2.0)\n", output[0]);
    pr_info("  output[1] = %f (expected: 12.0)\n", output[1]);
}

static void test_memory_layout(void) {
    test_block_q4_K blocks[2];
    void *ptr = blocks;
    int i;
    
    pr_info("\n=== Testing Memory Layout ===\n");
    pr_info("sizeof(test_block_q4_K) = %zu (expected: 144)\n", sizeof(test_block_q4_K));
    pr_info("Block alignment:\n");
    pr_info("  d offset: %zu\n", offsetof(test_block_q4_K, d));
    pr_info("  dmin offset: %zu\n", offsetof(test_block_q4_K, dmin));
    pr_info("  scales offset: %zu\n", offsetof(test_block_q4_K, scales));
    pr_info("  qs offset: %zu\n", offsetof(test_block_q4_K, qs));
    
    // Show first few bytes
    pr_info("First 16 bytes of memory:\n");
    for (i = 0; i < 16; i++) {
        pr_info("  byte[%d] = 0x%02x\n", i, ((uint8_t*)ptr)[i]);
    }
}

static void benchmark_fpu_overhead(void) {
    int i, iterations = 1000;
    ktime_t start, end;
    s64 no_fpu_ns, with_fpu_ns;
    volatile float dummy = 1.0f;
    
    pr_info("\n=== Benchmarking FPU Overhead ===\n");
    
    // Benchmark without FPU guards
    start = ktime_get();
    for (i = 0; i < iterations; i++) {
        dummy = dummy * 1.01f;
    }
    end = ktime_get();
    no_fpu_ns = ktime_to_ns(ktime_sub(end, start));
    
    // Benchmark with FPU guards
    start = ktime_get();
    for (i = 0; i < iterations; i++) {
        kernel_fpu_begin();
        dummy = dummy * 1.01f;
        kernel_fpu_end();
    }
    end = ktime_get();
    with_fpu_ns = ktime_to_ns(ktime_sub(end, start));
    
    pr_info("Time for %d iterations:\n", iterations);
    pr_info("  Without FPU guards: %lld ns (%lld ns/iter)\n", 
            no_fpu_ns, no_fpu_ns / iterations);
    pr_info("  With FPU guards: %lld ns (%lld ns/iter)\n", 
            with_fpu_ns, with_fpu_ns / iterations);
    pr_info("  Overhead: %lld ns/iteration\n", 
            (with_fpu_ns - no_fpu_ns) / iterations);
}

static int __init test_init(void) {
    pr_info("Q4_K Dequantization Test Module Loading\n");
    
    test_known_dequantization();
    test_memory_layout();
    benchmark_fpu_overhead();
    
    return 0;
}

static void __exit test_exit(void) {
    pr_info("Q4_K Dequantization Test Module Unloading\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Q4_K Dequantization Test");