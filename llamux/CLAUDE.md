# Llamux Project - Claude Development Guide

## CRITICAL REQUIREMENT
**NO WORKAROUNDS OR HALF-MEASURES ALLOWED**

We MUST get the real TinyLlama-1.1B model running in kernel space. This means:
- The GGUF parser must correctly parse the entire model file
- All weights must be loaded properly into kernel memory
- The inference engine must run actual transformer computations
- No mock models, no shortcuts, no "good enough" solutions

We will troubleshoot and debug until it works 100% correctly.

## Project Overview

**Llamux** is an ambitious project to integrate a Large Language Model (TinyLlama-1.1B) directly into the Linux kernel, creating the world's first AI-powered operating system at the kernel level. This enables natural language system management, AI-driven resource optimization, and intelligent kernel operations.

## System Information

### Development Machine
- **CPU**: AMD Ryzen 9 7950X3D 16-Core Processor (32 threads)
- **RAM**: 124GB DDR5
- **Storage**: 1.8TB (325GB available)
- **OS**: Ubuntu 24.04 LTS (kernel 6.8.0-60-generic)
- **Architecture**: x86_64
- **GPU**: None (CPU-only inference)

### Key Capabilities
- 32 CPU threads for parallel compilation
- Massive RAM allows loading full models in memory
- AVX2/AVX-512 support for SIMD optimizations
- Ideal for kernel development and testing

## Current Project Status

### Phase Progress
- **Phase 0**: ✅ Complete - Project setup and planning
- **Phase 1**: 🔄 Partially Complete - Development environment
- **Phase 2**: 🔄 Partially Complete - Kernel module development
- **Phase 3**: 🔄 In Progress - GGML Integration
- **Phase 4**: 📅 Current Sprint - Real weights and optimization

### Implementation Status
- **Infrastructure**: 90% - Module framework, proc interface, memory management
- **Parsing**: 60% - GGUF header parsing works, weight loading incomplete
- **Inference**: 10% - Mock implementation only, no real neural network
- **Integration**: 70% - Kernel hooks and interfaces ready

## Key Components

### 1. Kernel Module (`kernel/llama_core/`)
- **llama_core.c**: Main module with initialization and proc interface
- **gguf_parser.c**: GGUF model file parser (partial)
- **memory_reserve.c**: Boot-time memory reservation (2GB)
- **ggml_kernel.c**: Tensor operations (basic implementation)
- **tokenizer.c**: Simple tokenization (word-based)
- **llama_model.c**: Model structure and mock inference
- **llama_proc.c**: /proc/llamux interface

### 2. Critical Missing Pieces
1. **Transformer Implementation**: No attention mechanism
2. **Weight Loading**: Cannot load actual model weights
3. **Quantization**: No support for Q4_K format
4. **SIMD Optimization**: No vectorized operations
5. **Real Inference**: Currently generates random output

## Next Development Steps

### Immediate Priority (This Week)
1. **Complete GGUF Parser**
   - Implement tensor data loading
   - Add quantization format support
   - Test with real TinyLlama model

2. **Implement Core Transformer**
   - Multi-head attention mechanism
   - RoPE position embeddings
   - Layer normalization
   - Feedforward network

3. **Enable Real Inference**
   - Load actual model weights
   - Implement forward pass
   - Add KV-cache for efficiency

### Next Sprint
4. **Performance Optimization**
   - Add AVX2/AVX-512 SIMD
   - Implement int8 quantization
   - Optimize memory access patterns

5. **Build Bootable System**
   - Custom kernel with Llamux built-in
   - Create Arch-based ISO
   - Boot directly into AI Linux

## Development Commands

### Building the Module
```bash
cd /root/Idea/llamux
make clean && make
```

### Loading the Module
```bash
sudo insmod kernel/llama_core/llama_core.ko
# Check status
cat /proc/llamux/status
```

### Testing Inference
```bash
echo "Hello AI" > /proc/llamux/prompt
cat /proc/llamux/prompt
```

### Monitoring
```bash
dmesg | grep llamux
journalctl -f | grep llamux
```

## Architecture Decisions

1. **Model**: TinyLlama-1.1B Q4_K_M (637MB) - Small enough for kernel
2. **Memory**: 2GB reserved at boot via `llamux_mem=2G` parameter
3. **Interface**: /proc filesystem for user interaction
4. **Threading**: Dedicated kernel thread for inference
5. **Safety**: Synchronous operations to avoid kernel instability

## Current Challenges

1. **Kernel Constraints**: Limited to kernel-safe operations
2. **Memory Management**: kmalloc limited to 4MB chunks
3. **Floating Point**: Requires careful FPU state management
4. **Debugging**: Limited tools in kernel space

## Testing Strategy

1. **Unit Tests**: Individual component testing
2. **Integration Tests**: Full pipeline validation
3. **Performance Tests**: Inference speed benchmarks
4. **Stability Tests**: Long-running kernel tests
5. **VM Testing**: QEMU/KVM for safe development

## Security Considerations

- Model runs with kernel privileges
- Need input sanitization
- Resource limits to prevent DoS
- Secure model loading mechanism

## Future Vision

- Natural language system control: `llama "optimize for gaming"`
- AI-powered troubleshooting: `llama "why is my system slow?"`
- Intelligent resource management
- Kernel panic haikus!
- Boot messages with personality

## How Claude Can Help

1. **Complete GGUF Parser**: Implement full weight loading
2. **Build Transformer**: Implement attention and FFN layers
3. **Optimize Performance**: Add SIMD and quantization
4. **Debug Issues**: Analyze kernel logs and crashes
5. **Test Coverage**: Expand test suite
6. **Documentation**: Improve code documentation

## Project Structure
```
llamux/
├── kernel/
│   ├── llama_core/     # Main LLM kernel module
│   ├── llama_mm/       # Memory management (planned)
│   ├── llama_sched/    # Scheduler integration (planned)
│   └── llama_sec/      # Security module (planned)
├── models/             # LLM model storage
├── userspace/          # User tools (lsh, llama-mon)
├── scripts/            # Build and setup scripts
├── tests/              # Test suite
└── docs/               # Documentation
```

## Important Files
- `/root/Idea/llamux/DEVELOPMENT_PLAN.md` - Master development plan
- `/root/Idea/llamux/kernel/llama_core/llama_model.c` - Core inference (needs work)
- `/root/Idea/llamux/kernel/llama_core/ggml_kernel.c` - Tensor ops (needs expansion)

---
*Generated for Claude development context - Updated after full project audit*