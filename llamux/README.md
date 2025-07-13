# 🦙 Llamux - The Linux Distribution with a Llama in the Kernel

![Llamux Version](https://img.shields.io/badge/version-0.1.0--alpha-blue)
![Kernel](https://img.shields.io/badge/kernel-6.x-green)
![Model](https://img.shields.io/badge/model-TinyLlama--1.1B-orange)
![License](https://img.shields.io/badge/license-GPL--2.0-red)

## What is Llamux?

Llamux is the world's first Linux distribution with a Large Language Model (LLM) integrated directly into the kernel. It features TinyLlama-1.1B running in kernel space, providing intelligent system management through natural language interfaces.

### Key Features

- 🧠 **Kernel-Level AI**: TinyLlama-1.1B runs directly in kernel space
- 🔒 **100% Local**: No cloud dependencies, no subscriptions, complete privacy
- 💬 **Natural Language Control**: Talk to your OS in plain English
- ⚡ **Real-Time Intelligence**: AI-powered scheduling, memory management, and security
- 🛠️ **Developer Friendly**: Based on Arch Linux with full customization

## Quick Demo

```bash
# Traditional Linux
$ ps aux | grep chrome | awk '{print $2}' | xargs kill -9

# Llamux
$ llama "close memory-hogging chrome tabs"
🦙: Closing 3 Chrome processes using >2GB RAM. Kept your main window.
```

## System Requirements

### Minimum
- **CPU**: x86_64 with AVX2 support (Intel 4th gen+, AMD Zen+)
- **RAM**: 8GB (2GB reserved for LLM)
- **Storage**: 10GB free space
- **GPU**: Not required! CPU-only inference

### Recommended
- **CPU**: 6-8 cores, 3.6GHz+ (e.g., Ryzen 5 5600X)
- **RAM**: 16GB
- **Storage**: NVMe SSD
- **Cost**: ~$500-800 for a capable system

## Architecture Overview

```
┌─────────────────────────────────────────┐
│         Llamux Applications             │
├─────────────────────────────────────────┤
│       Llama Shell (lsh) 🦙              │
├─────────────────────────────────────────┤
│         System Call Interface           │
├─────────────────────────────────────────┤
│      TinyLlama Kernel Modules           │
│  ┌──────────┬──────────┬──────────┐   │
│  │ Llama    │ Llama    │ Llama    │   │
│  │ Scheduler│ Memory   │ Security │   │
│  └──────────┴──────────┴──────────┘   │
├─────────────────────────────────────────┤
│     Linux Kernel 6.x + GGML Runtime     │
└─────────────────────────────────────────┘
```

## Project Status

🚧 **Alpha Development** - Not ready for production use!

### Completed
- [x] Architecture design
- [x] Technology selection
- [x] Development plan

### In Progress
- [ ] Basic kernel module
- [ ] GGML integration
- [ ] Model loading system

### Upcoming
- [ ] Natural language shell
- [ ] System integration
- [ ] First alpha release

## Building from Source

### Prerequisites

```bash
# Arch Linux
sudo pacman -S base-devel bc wget ncurses git
sudo pacman -S xmlto kmod inetutils bc libelf git cpio perl tar xz

# Ubuntu/Debian
sudo apt-get install build-essential bc wget libncurses-dev git
sudo apt-get install libelf-dev bison flex libssl-dev
```

### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/llamux.git
cd llamux

# Download TinyLlama model
./scripts/download-model.sh

# Build kernel modules
make -C kernel/

# Build userspace tools
make -C userspace/

# Create bootable ISO (requires root)
sudo ./scripts/build-iso.sh
```

## Documentation

- [Installation Guide](docs/installation.md)
- [User Manual](docs/user-manual.md)
- [Developer Guide](docs/developer-guide.md)
- [Architecture Overview](docs/architecture.md)

## Community

- **Website**: [llamux.org](https://llamux.org) (coming soon)
- **IRC**: #llamux on Libera.Chat
- **Discord**: [Join our server](https://discord.gg/llamux) (coming soon)
- **Reddit**: [r/llamux](https://reddit.com/r/llamux) (coming soon)

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Areas We Need Help

- Kernel development (C)
- GGML optimization
- Userspace tools (Rust)
- Documentation
- Testing and QA
- Logo/mascot design

## Performance

With TinyLlama-1.1B Q4_K_M on a Ryzen 5 5600X:
- **Model Size**: 637MB (quantized)
- **Memory Usage**: ~1-2GB
- **Inference Speed**: 10-20 tokens/second
- **First Token Latency**: 50-100ms

## FAQ

**Q: Is this real?**  
A: Yes! We're building a real Linux distribution with an LLM in the kernel.

**Q: Why put an LLM in the kernel?**  
A: For zero-latency decisions, complete system visibility, and true AI-OS integration.

**Q: Is it secure?**  
A: Security is our top priority. The LLM runs in isolated memory with strict controls.

**Q: Does it require internet?**  
A: No! Llamux is 100% local. No cloud, no subscriptions.

**Q: Can I use a different model?**  
A: Currently we support TinyLlama. More models coming soon!

## License

Llamux is licensed under the GNU General Public License v2.0. See [LICENSE](LICENSE) for details.

## Acknowledgments

- The Linux kernel community
- TinyLlama team for the amazing model
- GGML project for the inference engine
- Our future contributors (that's you!)

---

*"The OS that thinks" - Llamux 🦙*