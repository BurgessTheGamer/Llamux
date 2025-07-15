#!/bin/bash

# Llamux Development Environment Setup
# Sets up everything needed for kernel development without reboots

set -e

echo "🦙 Llamux Development Environment Setup"
echo "======================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."
    
    # Check CPU features
    if grep -q avx2 /proc/cpuinfo; then
        print_status "CPU supports AVX2"
    else
        print_warning "CPU does not support AVX2 - performance may be limited"
    fi
    
    # Check memory
    total_mem=$(free -g | awk '/^Mem:/{print $2}')
    if [ "$total_mem" -ge 8 ]; then
        print_status "Memory: ${total_mem}GB (sufficient)"
    else
        print_warning "Memory: ${total_mem}GB (minimum 8GB recommended)"
    fi
    
    # Check kernel headers
    kernel_version=$(uname -r)
    if [ -d "/lib/modules/$kernel_version/build" ]; then
        print_status "Kernel headers found for $kernel_version"
    else
        print_error "Kernel headers not found for $kernel_version"
        echo "Installing kernel headers..."
        apt-get install -y linux-headers-$(uname -r) || {
            print_error "Failed to install kernel headers"
            echo "You may need to install them manually for your distribution"
        }
    fi
}

# Set up kernel module development
setup_kernel_dev() {
    echo ""
    echo "Setting up kernel module development..."
    
    # Install required packages
    print_status "Installing development packages..."
    apt-get update
    apt-get install -y \
        build-essential \
        bc \
        kmod \
        cpio \
        flex \
        bison \
        libelf-dev \
        libssl-dev \
        libncurses5-dev \
        rsync \
        python3 \
        python3-pip
    
    # Set up kernel module signing (optional, for secure boot systems)
    if mokutil --sb-state 2>/dev/null | grep -q "SecureBoot enabled"; then
        print_warning "Secure Boot is enabled - kernel module signing may be required"
        echo "See: https://wiki.debian.org/SecureBoot"
    fi
}

# Download TinyLlama model
download_model() {
    echo ""
    echo "Downloading TinyLlama model..."
    
    cd /root/Llamux
    
    # Create models directory
    mkdir -p models
    
    # Check if model already exists
    if [ -f "models/tinyllama-1.1b-q4_k_m.gguf" ]; then
        print_status "Model already downloaded"
    else
        print_status "Downloading TinyLlama-1.1B Q4_K_M (637MB)..."
        wget -q --show-progress \
            https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
            -O models/tinyllama-1.1b-q4_k_m.gguf || {
            print_error "Failed to download model"
            echo "You can manually download from: https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF"
            return 1
        }
        print_status "Model downloaded successfully"
    fi
}

# Set up VS Code integration (if available)
setup_vscode() {
    echo ""
    echo "Setting up development tools..."
    
    # Create .vscode directory with helpful settings
    mkdir -p /root/Llamux/.vscode
    
    cat > /root/Llamux/.vscode/c_cpp_properties.json << 'EOF'
{
    "configurations": [
        {
            "name": "Linux Kernel",
            "includePath": [
                "${workspaceFolder}/**",
                "/lib/modules/$(uname -r)/build/include",
                "/lib/modules/$(uname -r)/build/include/uapi",
                "/lib/modules/$(uname -r)/build/arch/x86/include",
                "/usr/include"
            ],
            "defines": [
                "__KERNEL__",
                "MODULE",
                "CONFIG_X86_64"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "gnu11",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
EOF

    cat > /root/Llamux/.vscode/tasks.json << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Kernel Module",
            "type": "shell",
            "command": "make",
            "args": ["-C", "llamux/kernel/llama_core"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Clean Build",
            "type": "shell",
            "command": "make",
            "args": ["-C", "llamux/kernel/llama_core", "clean"],
            "group": "build"
        },
        {
            "label": "Test Module",
            "type": "shell",
            "command": "sudo",
            "args": ["./test-kernel.sh", "1"],
            "group": "test"
        }
    ]
}
EOF

    print_status "VS Code configuration created"
}

# Create development scripts
create_dev_scripts() {
    echo ""
    echo "Creating development helper scripts..."
    
    # Quick build script
    cat > /root/Llamux/build.sh << 'EOF'
#!/bin/bash
echo "🔨 Building Llamux kernel modules..."
cd "$(dirname "$0")"
make -C llamux/kernel/llama_core clean
make -C llamux/kernel/llama_core
echo "✅ Build complete!"
EOF
    chmod +x /root/Llamux/build.sh
    
    # Module test script
    cat > /root/Llamux/test-module.sh << 'EOF'
#!/bin/bash
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (use sudo)"
    exit 1
fi

cd "$(dirname "$0")/llamux/kernel/llama_core"

# Remove if loaded
lsmod | grep -q llama_core && rmmod llama_core

# Insert module
echo "Loading llama_core module..."
insmod llama_core.ko

# Check status
if lsmod | grep -q llama_core; then
    echo "✅ Module loaded!"
    echo ""
    echo "Recent kernel messages:"
    dmesg | tail -20 | grep -E "(llama|Llamux)" || echo "No Llamux messages found"
    
    if [ -f /proc/llama_status ]; then
        echo ""
        echo "Llama status:"
        cat /proc/llama_status
    fi
    
    echo ""
    echo "Module info:"
    modinfo llama_core.ko
    
    # Cleanup
    echo ""
    read -p "Press Enter to unload module..."
    rmmod llama_core
    echo "Module unloaded"
else
    echo "❌ Failed to load module"
    echo "Check dmesg for errors:"
    dmesg | tail -10
fi
EOF
    chmod +x /root/Llamux/test-module.sh
    
    print_status "Helper scripts created"
}

# Create a safe testing VM setup
create_vm_setup() {
    echo ""
    echo "Creating VM testing setup..."
    
    cat > /root/Llamux/create-test-vm.sh << 'EOF'
#!/bin/bash
# Creates a minimal VM for testing Llamux kernel

echo "Creating Llamux test VM..."

# Create VM directory
mkdir -p vm-test
cd vm-test

# Create a 4GB disk image
qemu-img create -f qcow2 llamux-test.qcow2 4G

# Download Alpine Linux (minimal distro)
if [ ! -f alpine.iso ]; then
    wget https://dl-cdn.alpinelinux.org/alpine/v3.19/releases/x86_64/alpine-virt-3.19.0-x86_64.iso -O alpine.iso
fi

# Create start script
cat > start-vm.sh << 'EOFVM'
#!/bin/bash
qemu-system-x86_64 \
    -m 2048 \
    -smp 2 \
    -hda llamux-test.qcow2 \
    -cdrom alpine.iso \
    -boot d \
    -enable-kvm \
    -nographic \
    -serial mon:stdio
EOFVM
chmod +x start-vm.sh

echo "✅ VM setup complete!"
echo ""
echo "To start the VM: cd vm-test && ./start-vm.sh"
echo "Install Alpine, then copy kernel modules for testing"
EOF
    chmod +x /root/Llamux/create-test-vm.sh
    
    print_status "VM testing setup created"
}

# Main setup flow
main() {
    echo ""
    
    # Check if running as root
    if [ "$EUID" -ne 0 ]; then 
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
    
    # Run setup steps
    check_prerequisites
    setup_kernel_dev
    download_model
    setup_vscode
    create_dev_scripts
    create_vm_setup
    
    echo ""
    echo "======================================"
    print_status "Development environment setup complete!"
    echo ""
    echo "🦙 Next steps:"
    echo "  1. Build the kernel module: ./build.sh"
    echo "  2. Test locally (careful!): sudo ./test-module.sh"
    echo "  3. Test in container: ./test-kernel.sh"
    echo "  4. Create a test VM: ./create-test-vm.sh"
    echo ""
    echo "📚 Documentation:"
    echo "  - Architecture: llamux/docs/architecture.md"
    echo "  - Developer Guide: llamux/docs/developer-guide.md"
    echo "  - AGENTS.md for AI assistant guidelines"
    echo ""
    echo "Happy hacking! 🚀"
}

# Run main function
main