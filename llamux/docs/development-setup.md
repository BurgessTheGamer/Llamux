# Llamux Development Environment Setup

This guide will help you set up a complete development environment for Llamux.

## Table of Contents
1. [System Requirements](#system-requirements)
2. [Base System Setup](#base-system-setup)
3. [Development Tools](#development-tools)
4. [Virtual Machine Setup](#virtual-machine-setup)
5. [Building Llamux](#building-llamux)
6. [Testing](#testing)
7. [Troubleshooting](#troubleshooting)

## System Requirements

### Host Machine (Development)
- **CPU**: x86_64 with virtualization support (Intel VT-x or AMD-V)
- **RAM**: 16GB minimum (8GB for host + 8GB for VMs)
- **Storage**: 50GB free space
- **OS**: Linux (preferably Arch Linux or Ubuntu 22.04+)

### Target Machine (Running Llamux)
- **CPU**: x86_64 with AVX2 support
- **RAM**: 8GB minimum
- **Storage**: 10GB free space

## Base System Setup

### Option 1: Arch Linux (Recommended)

```bash
# Update system
sudo pacman -Syu

# Install base development tools
sudo pacman -S base-devel git vim

# Install kernel development packages
sudo pacman -S linux-headers bc wget ncurses
sudo pacman -S xmlto kmod inetutils bc libelf git cpio perl tar xz

# Install virtualization tools
sudo pacman -S qemu-full virt-manager libvirt edk2-ovmf

# Install Rust toolchain (for userspace tools)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

### Option 2: Ubuntu/Debian

```bash
# Update system
sudo apt update && sudo apt upgrade

# Install base development tools
sudo apt install build-essential git vim

# Install kernel development packages
sudo apt install linux-headers-$(uname -r) bc wget libncurses-dev
sudo apt install bison flex libssl-dev libelf-dev

# Install virtualization tools
sudo apt install qemu-kvm virt-manager libvirt-daemon-system

# Install Rust toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

## Development Tools

### Essential Tools

```bash
# Code editors (choose one)
sudo pacman -S code  # VS Code
sudo pacman -S neovim  # Neovim

# Debugging tools
sudo pacman -S gdb strace

# Performance analysis
sudo pacman -S perf valgrind

# Documentation tools
sudo pacman -S doxygen graphviz
```

### Kernel Development Specific

```bash
# Kernel source code navigation
sudo pacman -S cscope ctags

# Kernel configuration
sudo pacman -S menuconfig

# Module utilities
sudo pacman -S kmod
```

## Virtual Machine Setup

### 1. Create Llamux Development VM

```bash
# Create VM disk
qemu-img create -f qcow2 llamux-dev.qcow2 20G

# Download Arch Linux ISO
wget https://mirror.rackspace.com/archlinux/iso/latest/archlinux-x86_64.iso

# Start VM for installation
qemu-system-x86_64 \
    -enable-kvm \
    -m 4G \
    -cpu host \
    -smp 4 \
    -drive file=llamux-dev.qcow2,format=qcow2 \
    -cdrom archlinux-x86_64.iso \
    -boot d
```

### 2. VM Configuration Script

Create `scripts/start-vm.sh`:

```bash
#!/bin/bash
# Start Llamux development VM

qemu-system-x86_64 \
    -enable-kvm \
    -m 4G \
    -cpu host,+avx2 \
    -smp 4 \
    -drive file=llamux-dev.qcow2,format=qcow2 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device e1000,netdev=net0 \
    -display gtk \
    -monitor stdio
```

### 3. Snapshot for Testing

```bash
# Create base snapshot
qemu-img snapshot -c base llamux-dev.qcow2

# Revert to snapshot after testing
qemu-img snapshot -a base llamux-dev.qcow2
```

## Building Llamux

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/llamux.git
cd llamux
```

### 2. Download Model

```bash
./scripts/download-model.sh
```

### 3. Build Everything

```bash
# Using make
make all

# Or using build script
./scripts/build-all.sh
```

### 4. Build Individual Components

```bash
# Build kernel modules only
make kernel

# Build specific module
cd kernel/llama_core
make

# Build with debug symbols
make CFLAGS="-g -DDEBUG"
```

## Testing

### 1. Test Kernel Module

```bash
# Load module
sudo insmod kernel/llama_core/llama_core.ko

# Check if loaded
lsmod | grep llama

# View kernel messages
sudo dmesg | grep -i llamux

# Check proc interface
cat /proc/llamux/status

# Unload module
sudo rmmod llama_core
```

### 2. Automated Testing

```bash
# Run all tests
make test

# Run specific module test
cd kernel/llama_core
make test
```

### 3. Debugging Kernel Panics

```bash
# Enable kernel debugging
echo 8 > /proc/sys/kernel/printk

# Capture kernel panic
echo c > /proc/sysrq-trigger  # WARNING: This crashes the system!
```

## Development Workflow

### 1. Typical Development Cycle

```bash
# 1. Make changes
vim kernel/llama_core/llama_core.c

# 2. Build
make -C kernel/llama_core

# 3. Test in VM
scp kernel/llama_core/llama_core.ko vm:/tmp/
ssh vm "sudo insmod /tmp/llama_core.ko"

# 4. Check logs
ssh vm "sudo dmesg | tail -20"

# 5. Iterate
```

### 2. Git Workflow

```bash
# Create feature branch
git checkout -b feature/implement-inference

# Make changes and commit
git add -A
git commit -m "feat: Add basic inference loop"

# Push to remote
git push origin feature/implement-inference
```

## Troubleshooting

### Common Issues

#### 1. Module Build Fails
```bash
# Check kernel version matches headers
uname -r
ls /lib/modules/

# Install correct headers
sudo pacman -S linux-headers
```

#### 2. Module Won't Load
```bash
# Check for errors
sudo dmesg | tail

# Check module dependencies
modinfo llama_core.ko

# Force load (dangerous!)
sudo insmod llama_core.ko force=1
```

#### 3. VM Performance Issues
```bash
# Check KVM is enabled
lsmod | grep kvm

# Enable nested virtualization
echo "options kvm_intel nested=1" | sudo tee /etc/modprobe.d/kvm-intel.conf
```

### Kernel Debugging

```bash
# Enable all kernel debug messages
echo "kernel.printk = 8 8 8 8" | sudo tee -a /etc/sysctl.conf

# Use netconsole for remote debugging
modprobe netconsole netconsole=@/eth0,@192.168.1.100/

# Enable magic SysRq
echo 1 > /proc/sys/kernel/sysrq
```

## Advanced Topics

### Cross-Compilation

```bash
# For ARM64
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
make -C kernel/llama_core
```

### Performance Profiling

```bash
# Profile module loading
perf record -g insmod llama_core.ko
perf report

# Trace kernel functions
sudo trace-cmd record -p function_graph -g llama_init
trace-cmd report
```

## Next Steps

1. Complete Phase 1 of the development plan
2. Implement GGML tensor operations
3. Port inference engine to kernel space
4. Create userspace tools
5. Build bootable ISO

## Resources

- [Linux Kernel Development](https://www.kernel.org/doc/html/latest/)
- [Kernel Newbies](https://kernelnewbies.org/)
- [GGML Documentation](https://github.com/ggerganov/ggml)
- [Llamux GitHub](https://github.com/yourusername/llamux)

---

Happy hacking! 🦙