#include <stdio.h>
#include <stdint.h>
#include <math.h>

// Correct FP16 to FP32 conversion
float fp16_to_fp32(uint16_t h) {
    union { uint32_t u; float f; } o;
    
    uint32_t sign = (h >> 15) & 1;
    uint32_t exp = (h >> 10) & 0x1f;
    uint32_t mant = h & 0x3ff;
    
    if (exp == 0) {
        // Zero or subnormal
        if (mant == 0) {
            // Zero
            o.u = sign << 31;
        } else {
            // Subnormal - convert to normalized
            exp = 1;
            while ((mant & 0x400) == 0) {
                mant <<= 1;
                exp--;
            }
            mant &= 0x3ff;
            o.u = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        // Inf or NaN
        o.u = (sign << 31) | (0xff << 23) | (mant << 13);
    } else {
        // Normal number
        o.u = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
    }
    
    return o.f;
}

int main() {
    // Test values from the debug output
    uint16_t test_values[] = {
        0x58b7,  // Should be ~99.4
        0xff4a,  // Negative value
        0xf51f,  // Large negative
        0x3C00,  // 1.0
        0xBC00,  // -1.0
        0x0000,  // 0.0
        0x8000,  // -0.0
    };
    
    printf("Testing FP16 to FP32 conversion:\n");
    for (int i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
        float result = fp16_to_fp32(test_values[i]);
        printf("0x%04x -> %f\n", test_values[i], result);
    }
    
    return 0;
}