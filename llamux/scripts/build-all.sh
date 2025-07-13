#!/bin/bash
#
# Build all Llamux components
# This is the main build script for the entire project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "🦙 Llamux Build System"
echo "====================="
echo ""

# Function to print colored status
status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Check for required tools
status "Checking build dependencies..."

check_tool() {
    if ! command -v $1 &> /dev/null; then
        error "$1 is not installed!"
        exit 1
    fi
}

check_tool make
check_tool gcc
check_tool cargo  # For Rust userspace tools

# Check kernel headers
if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    error "Kernel headers not found for $(uname -r)"
    error "Install with: sudo pacman -S linux-headers (Arch)"
    error "           or: sudo apt-get install linux-headers-$(uname -r) (Debian/Ubuntu)"
    exit 1
fi

# Build kernel modules
status "Building kernel modules..."
cd "$PROJECT_ROOT/kernel"

for module in llama_core llama_sched llama_mm llama_sec; do
    if [ -d "$module" ] && [ -f "$module/Makefile" ]; then
        status "Building $module..."
        make -C "$module" clean
        make -C "$module"
    else
        warning "Skipping $module (not yet implemented)"
    fi
done

# Build userspace tools
status "Building userspace tools..."
cd "$PROJECT_ROOT/userspace"

# Build Rust projects
for tool in lsh llama-mon llama-config; do
    if [ -d "$tool" ] && [ -f "$tool/Cargo.toml" ]; then
        status "Building $tool..."
        cd "$tool"
        cargo build --release
        cd ..
    else
        warning "Skipping $tool (not yet implemented)"
    fi
done

# Create build artifacts directory
BUILD_DIR="$PROJECT_ROOT/build"
mkdir -p "$BUILD_DIR"/{modules,bin,lib}

# Collect built artifacts
status "Collecting build artifacts..."

# Kernel modules
find "$PROJECT_ROOT/kernel" -name "*.ko" -exec cp {} "$BUILD_DIR/modules/" \; 2>/dev/null || true

# Userspace binaries
find "$PROJECT_ROOT/userspace" -path "*/target/release/*" -type f -executable \
    -not -name "*.d" -not -name "*.rlib" \
    -exec cp {} "$BUILD_DIR/bin/" \; 2>/dev/null || true

# Summary
echo ""
echo "🦙 Build Summary"
echo "==============="

# Count artifacts
MODULE_COUNT=$(find "$BUILD_DIR/modules" -name "*.ko" 2>/dev/null | wc -l)
BIN_COUNT=$(find "$BUILD_DIR/bin" -type f 2>/dev/null | wc -l)

echo "Kernel modules built: $MODULE_COUNT"
if [ $MODULE_COUNT -gt 0 ]; then
    find "$BUILD_DIR/modules" -name "*.ko" -exec basename {} \; | sed 's/^/  - /'
fi

echo ""
echo "Userspace tools built: $BIN_COUNT"
if [ $BIN_COUNT -gt 0 ]; then
    find "$BUILD_DIR/bin" -type f -exec basename {} \; | sed 's/^/  - /'
fi

echo ""
if [ $MODULE_COUNT -gt 0 ] || [ $BIN_COUNT -gt 0 ]; then
    status "Build complete! Artifacts in: $BUILD_DIR"
else
    warning "No artifacts built. Some components may not be implemented yet."
fi

echo ""
echo "🦙 Next steps:"
echo "  1. Download model: ./scripts/download-model.sh"
echo "  2. Test module: cd kernel/llama_core && make test"
echo "  3. Build ISO: sudo ./scripts/build-iso.sh (coming soon)"