#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/fpu/api.h>

// Test FP16 to FP32 conversion

static float fp16_to_fp32_correct(uint16_t h) {
    uint32_t sign = (h >> 15) & 0x1;
    uint32_t exponent = (h >> 10) & 0x1f;
    uint32_t mantissa = h & 0x3ff;
    
    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            return sign ? -0.0f : 0.0f;
        } else {
            // Subnormal
            float value = (mantissa / 1024.0f) * (1.0f / 16384.0f);
            return sign ? -value : value;
        }
    } else if (exponent == 31) {
        // Inf/NaN
        if (mantissa == 0) {
            return sign ? -INFINITY : INFINITY;
        } else {
            return 0.0f / 0.0f; // NaN
        }
    }
    
    // Normal number
    int32_t e = exponent - 15;
    float value = (1.0f + mantissa / 1024.0f);
    
    // Calculate 2^e
    if (e >= 0) {
        while (e > 0) {
            value *= 2.0f;
            e--;
        }
    } else {
        while (e < 0) {
            value /= 2.0f;
            e++;
        }
    }
    
    return sign ? -value : value;
}

static void test_fp16_conversions(void) {
    struct {
        uint16_t fp16;
        float expected;
        const char *desc;
    } test_cases[] = {
        {0x0000, 0.0f, "positive zero"},
        {0x8000, -0.0f, "negative zero"},
        {0x3C00, 1.0f, "one"},
        {0xBC00, -1.0f, "negative one"},
        {0x4000, 2.0f, "two"},
        {0x3800, 0.5f, "half"},
        {0x4200, 3.0f, "three"},
        {0x3E00, 1.5f, "one and half"},
        {0x7C00, INFINITY, "positive infinity"},
        {0xFC00, -INFINITY, "negative infinity"},
        {0x3555, 0.333251953125f, "~1/3"},
        {0x4900, 10.0f, "ten"},
    };
    
    pr_info("=== Testing FP16 to FP32 Conversion ===\n");
    
    kernel_fpu_begin();
    
    for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
        float result = fp16_to_fp32_correct(test_cases[i].fp16);
        float direct_cast = (float)test_cases[i].fp16;
        
        pr_info("FP16: 0x%04x (%s)\n", test_cases[i].fp16, test_cases[i].desc);
        pr_info("  Correct conversion: %f\n", result);
        pr_info("  Direct cast (WRONG): %f\n", direct_cast);
        pr_info("  Expected: %f\n", test_cases[i].expected);
        pr_info("  Match: %s\n\n", 
                (result == test_cases[i].expected) ? "YES" : "NO");
    }
    
    kernel_fpu_end();
    
    // Test what happens in current dequantization
    pr_info("=== Current Bug Demonstration ===\n");
    uint16_t d_fp16 = 0x3C00; // 1.0 in fp16
    uint16_t dmin_fp16 = 0x3800; // 0.5 in fp16
    
    kernel_fpu_begin();
    
    // What the current code does (WRONG)
    float d_wrong = (float)d_fp16;
    float dmin_wrong = (float)dmin_fp16;
    
    // What it should do
    float d_correct = fp16_to_fp32_correct(d_fp16);
    float dmin_correct = fp16_to_fp32_correct(dmin_fp16);
    
    pr_info("d (fp16: 0x%04x):\n", d_fp16);
    pr_info("  Wrong (direct cast): %f\n", d_wrong);
    pr_info("  Correct: %f\n", d_correct);
    
    pr_info("dmin (fp16: 0x%04x):\n", dmin_fp16);
    pr_info("  Wrong (direct cast): %f\n", dmin_wrong);
    pr_info("  Correct: %f\n", dmin_correct);
    
    kernel_fpu_end();
}

static int __init test_init(void) {
    pr_info("FP16 Conversion Test Module Loading\n");
    test_fp16_conversions();
    return 0;
}

static void __exit test_exit(void) {
    pr_info("FP16 Conversion Test Module Unloading\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FP16 Conversion Test");