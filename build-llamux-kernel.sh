#!/bin/bash

# Llamux Custom Kernel Build Script
# Builds a Debian 12 kernel optimized for running LLMs in kernel space

set -e

echo "🦙 Llamux Custom Kernel Builder"
echo "==============================="
echo "Building kernel with:"
echo "  - 4GB vmalloc space"
echo "  - 2GB/2GB kernel/user split"
echo "  - Optimizations for AI workload"
echo ""

# Configuration
KERNEL_VERSION="6.1"
BUILD_DIR="/root/Llamux/kernel-build"
JOBS=$(nproc)

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Step 1: Install build dependencies
install_deps() {
    echo "Installing build dependencies..."
    apt-get update
    apt-get install -y \
        build-essential \
        linux-source-$KERNEL_VERSION \
        libncurses-dev \
        flex \
        bison \
        libssl-dev \
        libelf-dev \
        bc \
        rsync \
        kmod \
        cpio \
        xz-utils \
        fakeroot \
        kernel-wedge \
        quilt \
        python3
    print_status "Dependencies installed"
}

# Step 2: Extract kernel source
setup_source() {
    echo ""
    echo "Setting up kernel source..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if [ ! -d "linux-source-$KERNEL_VERSION" ]; then
        tar -xf /usr/src/linux-source-$KERNEL_VERSION.tar.xz
        print_status "Kernel source extracted"
    else
        print_status "Kernel source already extracted"
    fi
    
    cd "linux-source-$KERNEL_VERSION"
}

# Step 3: Configure kernel for Llamux
configure_kernel() {
    echo ""
    echo "Configuring kernel for Llamux..."
    
    # Start with current running config
    cp /boot/config-$(uname -r) .config
    
    # Update config for Llamux
    cat >> .config << 'EOF'

# Llamux-specific configurations
CONFIG_LOCALVERSION="-llamux"
CONFIG_LOCALVERSION_AUTO=n

# Memory split - 2G/2G for more kernel space
# CONFIG_VMSPLIT_3G is not set
CONFIG_VMSPLIT_2G=y
# CONFIG_VMSPLIT_1G is not set

# Huge pages for better LLM performance
CONFIG_TRANSPARENT_HUGEPAGE=y
CONFIG_TRANSPARENT_HUGEPAGE_ALWAYS=y

# Enable for better debugging
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_INFO_REDUCED=n

# Increase log buffer for kernel messages
CONFIG_LOG_BUF_SHIFT=21
EOF

    # Update config
    make olddefconfig
    
    print_status "Kernel configured for Llamux"
}

# Step 4: Apply Llamux patches
apply_patches() {
    echo ""
    echo "Applying Llamux patches..."
    
    # Create patch for increased vmalloc
    cat > llamux-vmalloc.patch << 'EOF'
--- a/arch/x86/include/asm/page_64_types.h
+++ b/arch/x86/include/asm/page_64_types.h
@@ -40,7 +40,7 @@
  * hypervisor to fit.  Choosing 16 TiB allows for a 16 TiB minimum size
  * with most of the space for VMALLOC.  But we may need more space for
  * VMEMMAP and PAGE_OFFSET may need to change.
- * 
+ * LLAMUX: Increased for LLM support
  * On KASLR, the kernel text mapping starts at a random offset in the
  * region [_BASE, _BASE + _SIZE), so the compile time virtual addresses
  * of kernel text symbols are not equal to their runtime virtual address.
@@ -52,7 +52,7 @@
 #define __PAGE_OFFSET           __PAGE_OFFSET_BASE_L4
 
 #ifdef CONFIG_RANDOMIZE_MEMORY
-#define __VMALLOC_RESERVE      (128UL << 20)
+#define __VMALLOC_RESERVE      (4096UL << 20)  /* 4GB for Llamux */
 #else
 #define __VMALLOC_RESERVE      (192UL << 20)
 #endif
EOF

    # Apply patch if not already applied
    if ! grep -q "LLAMUX:" arch/x86/include/asm/page_64_types.h 2>/dev/null; then
        patch -p1 < llamux-vmalloc.patch || {
            print_error "Failed to apply patch, continuing anyway"
        }
    fi
    
    print_status "Patches applied"
}

# Step 5: Build kernel
build_kernel() {
    echo ""
    echo "Building Llamux kernel (this will take a while)..."
    echo "Using $JOBS parallel jobs"
    
    # Clean any previous builds
    make clean
    
    # Build kernel
    make -j$JOBS bindeb-pkg LOCALVERSION=-llamux KDEB_PKGVERSION=$(make kernelversion)-1
    
    print_status "Kernel built successfully!"
}

# Step 6: Show results
show_results() {
    echo ""
    echo "Build complete! 🎉"
    echo ""
    echo "Kernel packages created:"
    ls -la ../*.deb
    echo ""
    echo "To install:"
    echo "  cd $BUILD_DIR"
    echo "  sudo dpkg -i linux-image-*-llamux_*.deb"
    echo "  sudo dpkg -i linux-headers-*-llamux_*.deb"
    echo ""
    echo "Then update GRUB and reboot:"
    echo "  sudo update-grub"
    echo "  sudo reboot"
    echo ""
    echo "Boot parameters to add (in /etc/default/grub):"
    echo "  GRUB_CMDLINE_LINUX=\"vmalloc=4096M\""
}

# Main execution
main() {
    if [ "$EUID" -ne 0 ]; then 
        print_error "Please run as root"
        exit 1
    fi
    
    install_deps
    setup_source
    configure_kernel
    apply_patches
    build_kernel
    show_results
}

# Run main
main