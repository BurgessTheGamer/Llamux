# Llamux Agent Guidelines

## Build Commands
- Build all: `make all` or `bash scripts/build-all.sh`
- Build kernel module: `make -C kernel/llama_core`
- Clean: `make clean`
- Test module: `sudo bash tests/test_module.sh`
- Test single component: `cd kernel/llama_core && make test`
- Download model: `make download-model`

## Code Style
- **Language**: C for kernel modules, Python for simulators
- **Headers**: Include GPL v2 license header and module description
- **Includes**: System headers first, then local headers (e.g., `"gguf_parser.h"`)
- **Naming**: snake_case for functions/variables, UPPERCASE for macros/constants
- **Prefixes**: Use `llamux_` or `llama_` for public functions
- **Error handling**: Always check return values, use kernel logging (pr_info, pr_err)
- **Memory**: Use kmalloc/kfree for kernel allocations, check for NULL
- **Logging**: Use pr_info with "🦙 Llamux:" prefix for user-visible messages
- **Comments**: Minimal inline comments, focus on function/struct documentation
- **Line length**: Keep under 100 characters when possible
- **Testing**: Run `make test` before commits, check dmesg for kernel messages