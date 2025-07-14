#!/bin/bash
#
# Build custom Linux kernel with Llamux built-in
#

set -e

# Configuration
KERNEL_VERSION="6.8.0"
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz"
BUILD_DIR="/tmp/llamux-kernel-build"
LLAMUX_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "🦙 Llamux Kernel Builder"
echo "======================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
    echo -e "${RED}Warning: Building kernel as root is not recommended${NC}"
fi

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Download kernel source if not exists
if [ ! -f "linux-${KERNEL_VERSION}.tar.xz" ]; then
    echo -e "${YELLOW}Downloading Linux kernel ${KERNEL_VERSION}...${NC}"
    wget "$KERNEL_URL"
fi

# Extract kernel source
if [ ! -d "linux-${KERNEL_VERSION}" ]; then
    echo -e "${YELLOW}Extracting kernel source...${NC}"
    tar -xf "linux-${KERNEL_VERSION}.tar.xz"
fi

cd "linux-${KERNEL_VERSION}"

# Copy Llamux module into kernel tree
echo -e "${YELLOW}Integrating Llamux into kernel source...${NC}"
mkdir -p drivers/llamux
cp -r "$LLAMUX_DIR/kernel/llama_core/"* drivers/llamux/

# Create Kconfig for Llamux
cat > drivers/llamux/Kconfig << 'EOF'
config LLAMUX
	tristate "Llamux - AI in the Linux Kernel"
	default m
	help
	  This enables Llamux, which integrates TinyLlama AI model
	  directly into the Linux kernel for AI-powered system management.
	  
	  Say Y to build it into the kernel.
	  Say M to build it as a module.
	  Say N to exclude it.

config LLAMUX_DEBUG
	bool "Enable Llamux debug messages"
	depends on LLAMUX
	default y
	help
	  Enable debug messages for Llamux development.

config LLAMUX_MODEL_SIZE
	int "Reserved memory for AI model (MB)"
	depends on LLAMUX
	default 2048
	help
	  Amount of memory to reserve at boot for the AI model.
	  Default is 2048MB (2GB) for TinyLlama.
EOF

# Add Llamux to drivers Makefile
if ! grep -q "llamux" drivers/Makefile; then
    echo "obj-\$(CONFIG_LLAMUX) += llamux/" >> drivers/Makefile
fi

# Add Llamux to drivers Kconfig
if ! grep -q "llamux/Kconfig" drivers/Kconfig; then
    sed -i '/endmenu/i source "drivers/llamux/Kconfig"' drivers/Kconfig
fi

# Update Llamux Makefile for in-kernel build
cat > drivers/llamux/Makefile << 'EOF'
# Llamux kernel module
obj-$(CONFIG_LLAMUX) += llama_core.o

llama_core-y := main.o gguf_parser.o memory_reserve_simple.o \
                ggml_kernel.o tokenizer.o llama_model.o \
                llama_proc.o quantize.o

ccflags-y += -I$(src)
ccflags-$(CONFIG_LLAMUX_DEBUG) += -DLLAMUX_DEBUG

# SIMD optimizations
ccflags-y += -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2
ccflags-$(CONFIG_X86_64) += -mavx -mavx2 -mfma
EOF

# Configure kernel
echo -e "${YELLOW}Configuring kernel...${NC}"

# Start with current system config
if [ -f /boot/config-$(uname -r) ]; then
    cp /boot/config-$(uname -r) .config
else
    make defconfig
fi

# Enable Llamux
echo -e "${YELLOW}Enabling Llamux in kernel config...${NC}"
./scripts/config --enable CONFIG_LLAMUX
./scripts/config --enable CONFIG_LLAMUX_DEBUG
./scripts/config --set-val CONFIG_LLAMUX_MODEL_SIZE 2048

# Update config
make olddefconfig

# Show Llamux config
echo -e "${GREEN}Llamux configuration:${NC}"
grep LLAMUX .config

# Build kernel
echo -e "${YELLOW}Building kernel (this will take a while)...${NC}"
echo -e "${YELLOW}Using $(nproc) CPU cores${NC}"

# Uncomment to actually build (takes 30-60 minutes)
# make -j$(nproc) bzImage modules

echo -e "${GREEN}Kernel configuration complete!${NC}"
echo ""
echo "To build the kernel, run:"
echo "  cd $BUILD_DIR/linux-${KERNEL_VERSION}"
echo "  make -j\$(nproc) bzImage modules"
echo "  sudo make modules_install"
echo "  sudo make install"
echo ""
echo "The kernel will have Llamux built-in! 🦙"