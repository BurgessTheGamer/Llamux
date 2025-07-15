#!/bin/bash

# Run Llamux in a VM with NO REBOOTS REQUIRED!

set -e

echo "🦙 Llamux VM Runner - NO REBOOTS!"
echo "================================="
echo "Run Llamux in an isolated VM with custom kernel"
echo ""

VM_DIR="/root/Llamux/vm-environment"
mkdir -p $VM_DIR
cd $VM_DIR

# Step 1: Download a minimal Linux that supports custom kernels
if [ ! -f "alpine-virt.iso" ]; then
    echo "📥 Downloading Alpine Linux (minimal, 50MB)..."
    wget -q --show-progress https://dl-cdn.alpinelinux.org/alpine/v3.19/releases/x86_64/alpine-virt-3.19.0-x86_64.iso -O alpine-virt.iso
fi

# Step 2: Create VM disk
if [ ! -f "llamux-vm.qcow2" ]; then
    echo "💾 Creating VM disk..."
    qemu-img create -f qcow2 llamux-vm.qcow2 8G
fi

# Step 3: Create startup script with vmalloc
cat > start-llamux-vm.sh << 'EOF'
#!/bin/bash
echo "🚀 Starting Llamux VM..."
echo "   - 4GB RAM with 2GB for vmalloc"
echo "   - Custom kernel parameters"
echo "   - NO HOST REBOOT NEEDED!"
echo ""
echo "VM Controls:"
echo "  - Ctrl+A, X = Exit"
echo "  - Ctrl+A, C = QEMU Monitor"
echo ""

# Start QEMU with custom kernel parameters
qemu-system-x86_64 \
    -m 4096 \
    -smp 4 \
    -drive file=llamux-vm.qcow2,format=qcow2 \
    -cdrom alpine-virt.iso \
    -boot c \
    -enable-kvm \
    -nographic \
    -serial mon:stdio \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd /boot/initrd.img-$(uname -r) \
    -append "root=/dev/sda1 console=ttyS0 vmalloc=2048M" \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device e1000,netdev=net0
EOF
chmod +x start-llamux-vm.sh

# Step 4: Create setup script for inside the VM
cat > setup-in-vm.sh << 'EOF'
#!/bin/sh
echo "🦙 Setting up Llamux in VM..."

# Install build tools
apk add build-base linux-headers wget

# Create Llamux directory
mkdir -p /root/llamux
cd /root/llamux

# Copy kernel module source (you'll need to transfer this)
echo "📦 Ready for Llamux module!"
echo ""
echo "Next steps inside VM:"
echo "1. Transfer llama_core module source"
echo "2. Build with: make"
echo "3. Load with: insmod llama_core.ko"
echo ""
echo "The VM has vmalloc=2048M already set!"
EOF

# Step 5: Alternative - Use microVM with custom kernel
cat > run-microvm.sh << 'EOF'
#!/bin/bash
echo "🚀 Llamux MicroVM - Ultra Fast!"
echo ""

# Use Firecracker or QEMU microvm for even faster startup
qemu-system-x86_64 \
    -M microvm,x-option-roms=off,rtc=on \
    -no-acpi \
    -cpu host \
    -enable-kvm \
    -m 2G \
    -smp 2 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -append "console=ttyS0 vmalloc=2048M" \
    -initrd /boot/initrd.img-$(uname -r) \
    -serial stdio \
    -display none \
    -device virtio-blk-device,drive=test \
    -drive id=test,file=llamux-vm.qcow2,format=qcow2,if=none \
    -netdev user,id=testnet \
    -device virtio-net-device,netdev=testnet
EOF
chmod +x run-microvm.sh

# Step 6: Docker + QEMU hybrid approach
cat > Dockerfile.llamux-vm << 'EOF'
FROM debian:12

RUN apt-get update && apt-get install -y \
    qemu-system-x86 \
    qemu-utils \
    build-essential \
    linux-image-amd64 \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Copy Llamux
COPY llamux /llamux

# Create VM launcher
RUN echo '#!/bin/bash\n\
qemu-system-x86_64 \
    -m 2G \
    -kernel /boot/vmlinuz-* \
    -initrd /boot/initrd.img-* \
    -append "console=ttyS0 vmalloc=2048M" \
    -nographic \
    -serial mon:stdio' > /run-vm.sh && chmod +x /run-vm.sh

CMD ["/run-vm.sh"]
EOF

echo "✅ Llamux VM environment ready!"
echo ""
echo "🎯 THREE ways to run WITHOUT rebooting:"
echo ""
echo "1. Full VM with storage:"
echo "   ./start-llamux-vm.sh"
echo ""
echo "2. MicroVM (super fast):"
echo "   ./run-microvm.sh"
echo ""
echo "3. Docker + QEMU:"
echo "   docker build -f Dockerfile.llamux-vm -t llamux-vm /root/Llamux"
echo "   docker run -it --rm --privileged llamux-vm"
echo ""
echo "All options give you:"
echo "✅ Custom vmalloc settings"
echo "✅ Isolated environment"
echo "✅ NO HOST REBOOT NEEDED!"
echo "✅ Full kernel module support"