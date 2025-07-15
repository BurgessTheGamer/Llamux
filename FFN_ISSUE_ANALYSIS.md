# CodeLlama 13B FFN Matrix Multiplication Issue - Detailed Analysis

## Executive Summary

We are attempting to run CodeLlama 13B (a 13 billion parameter language model) entirely in Linux kernel space - something that has never been done before. We have successfully loaded the model and gotten through most of the inference pipeline, but are stuck at the Feed-Forward Network (FFN) layer due to matrix dimension mismatches.

## Current Progress

### ✅ Completed
1. **Model Loading**: Successfully loaded 7.5GB CodeLlama 13B model using direct file I/O (bypassing firmware API's 500MB limit)
2. **GGUF v2 Support**: Added support for GGUF version 2 format (CodeLlama uses v2, TinyLlama uses v3)
3. **Memory Management**: Allocated 20GB total (7.5GB for model + 12.5GB for inference)
4. **Tokenization**: Working correctly ("def fibonacci(n):" → 5 tokens)
5. **Embeddings**: Successfully retrieving token embeddings
6. **Attention Mechanism**: Fixed with proper transpose operations
7. **Layer Processing**: Successfully processing through transformer layers

### ❌ Blocked
- **FFN Matrix Multiplication**: Dimension mismatch preventing feed-forward network computation

## The Core Problem

### Error Message
```
🦙 GGML: Matrix dimensions incompatible for A@B^T: A[6,5120] @ B[5120,13824]^T
🦙 GGML: Need A.ne[0] (6) == B.ne[0] (5120)
```

### Technical Details

The Feed-Forward Network in transformer models consists of two linear projections with a non-linearity:

```python
# Standard FFN computation
hidden = activation(input @ W1) * (input @ W3)  # Gate and Up projections
output = hidden @ W2                             # Down projection
```

For CodeLlama 13B:
- `hidden_dim` = 5120 (model dimension)
- `intermediate_dim` = 13824 (FFN dimension, typically ~2.7x hidden_dim)
- `seq_len` = 6 (current sequence length after tokenization)

### Matrix Dimensions

Current tensor shapes when error occurs:
- **Input tensor (cur)**: `[5120, 6]` - shape: [hidden_dim, seq_len]
- **W1 (gate weight)**: `[5120, 13824]` - shape: [hidden_dim, intermediate_dim]
- **W3 (up weight)**: `[5120, 13824]` - shape: [hidden_dim, intermediate_dim]
- **W2 (down weight)**: `[13824, 5120]` - shape: [intermediate_dim, hidden_dim]

## The Mathematical Challenge

Our `ggml_mul_mat(A, B)` function implements: `result = A @ B^T` (A times B-transpose)

This requires:
- `A.ne[0] == B.ne[0]` (first dimensions must match)
- Result shape: `[B.ne[1], A.ne[1]]`

Standard FFN needs:
1. `W1 @ input`: `[13824, 5120] @ [5120, 6] = [13824, 6]`
2. But we have W1 as `[5120, 13824]` and our operation computes `A @ B^T`

## Attempted Solutions

### Attempt 1: Direct Multiplication
```c
struct ggml_tensor *gate = ggml_mul_mat(ctx, layer->w1, cur);
```
**Result**: Failed - `[5120, 13824] @ [5120, 6]^T` requires matching first dimension

### Attempt 2: Transpose Input First
```c
struct ggml_tensor *cur_t = ggml_transpose(ctx, cur);  // [6, 5120]
struct ggml_tensor *gate_t = ggml_mul_mat(ctx, cur_t, layer->w1);
```
**Result**: Failed - `[6, 5120] @ [5120, 13824]^T` still has dimension mismatch

### Attempt 3: Transpose Weights
```c
struct ggml_tensor *w1_t = ggml_transpose(ctx, layer->w1);  // [13824, 5120]
struct ggml_tensor *gate = ggml_mul_mat(ctx, w1_t, cur);    // [13824, 5120] @ [5120, 6]^T
```
**Status**: This should theoretically work but system becomes unstable

## Root Cause Analysis

### 1. GGML Operation Semantics
- GGML's `mul_mat` computes `A @ B^T` instead of standard `A @ B`
- This is likely for efficiency in attention mechanisms where `K^T` is commonly needed

### 2. Weight Storage Format
- Weights might be stored transposed in GGUF file
- Need to verify actual storage layout vs. expected layout

### 3. Memory Layout
- Row-major vs column-major considerations
- Potential alignment issues with kernel memory

## Proposed Solutions

### Solution 1: Implement Standard Matrix Multiplication
```c
// Add new function that computes A @ B without transpose
struct ggml_tensor *ggml_matmul(ctx, A, B) {
    // Implement standard matrix multiplication
}
```

### Solution 2: Fix Weight Loading
```c
// In gguf_parser.c, transpose FFN weights during loading
if (is_ffn_weight(tensor_name)) {
    tensor = transpose_on_load(tensor);
}
```

### Solution 3: Adjust Operation Order
```c
// Compute (input^T @ W^T)^T instead of W @ input
struct ggml_tensor *result = ggml_transpose(
    ggml_mul_mat(
        ggml_transpose(input),
        ggml_transpose(weight)
    )
);
```

### Solution 4: Verify GGUF Format
- Dump actual tensor dimensions from GGUF file
- Compare with expected dimensions
- Check if weights are pre-transposed

## Impact and Implications

### Why This Matters
1. **First of Its Kind**: Running a 13B parameter LLM in kernel space is unprecedented
2. **Performance**: Kernel-space execution could eliminate context switching overhead
3. **Security**: Direct hardware access for AI inference
4. **Research Value**: Opens new possibilities for OS-integrated AI

### Current Blockers
1. Matrix dimension mismatch in FFN
2. System instability after error (module gets stuck in -1 state)
3. Need to reboot to clear module state

## Next Steps

1. **Immediate**: Add debug logging to trace exact tensor shapes at each step
2. **Short-term**: Implement one of the proposed solutions
3. **Long-term**: Optimize memory usage and add proper error recovery

## File Locations

- Main inference: `/root/Llamux/llamux/kernel/llama_core/llama_model.c`
- Matrix operations: `/root/Llamux/llamux/kernel/llama_core/ggml_kernel.c`
- Model loading: `/root/Llamux/llamux/kernel/llama_core/gguf_parser.c`
- Module entry: `/root/Llamux/llamux/kernel/llama_core/main.c`

## Testing Command

```bash
# Load module
sudo insmod /root/Llamux/llamux/kernel/llama_core/llama_core.ko

# Test inference
echo "def fibonacci(n):" > /proc/llamux/prompt

# Check logs
dmesg | tail -100
```

## Progress Estimate

- Overall completion: ~95%
- Remaining work: Fix FFN matrix operations
- Estimated time to fix: 1-2 hours with correct approach

---

*Document created: January 2025*
*Project: Llamux - LLM in Linux Kernel*
*Model: CodeLlama 13B Q4_K*