#!/bin/bash

# Instant Llamux VM - Boot directly with kernel module

echo "🦙 Llamux Instant VM"
echo "==================="
echo "Boot a VM with Llamux pre-loaded!"
echo ""

VM_DIR="/root/Llamux/instant-vm"
mkdir -p $VM_DIR
cd $VM_DIR

# Create a minimal initramfs with Llamux
echo "🔧 Building Llamux initramfs..."

# Create initramfs structure
mkdir -p initramfs/{bin,dev,proc,sys,lib/modules,root}

# Copy static busybox
cp /bin/busybox initramfs/bin/
cd initramfs/bin
for cmd in sh ls cat echo mount umount insmod lsmod rmmod dmesg; do
    ln -sf busybox $cmd
done
cd ../..

# Copy kernel module
cp /root/Llamux/llamux/kernel/llama_core/llama_core.ko initramfs/lib/modules/

# Copy model to initramfs (smaller test file)
mkdir -p initramfs/lib/firmware/llamux
# Create a small test model file (not the full 638MB)
echo "TEST_MODEL_DATA" > initramfs/lib/firmware/llamux/tinyllama.gguf

# Create init script
cat > initramfs/init << 'EOF'
#!/bin/sh

# Mount essential filesystems
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

clear
echo ""
echo "🦙 Llamux Instant VM"
echo "==================="
echo ""
echo "Kernel: $(uname -r)"
echo "Cmdline: $(cat /proc/cmdline)"
echo ""

# Show vmalloc info
echo "Memory info:"
grep -E "MemTotal|MemFree|VmallocTotal|VmallocUsed" /proc/meminfo
echo ""

# Load Llamux module
echo "Loading Llamux kernel module..."
cd /lib/modules
if insmod llama_core.ko 2>&1; then
    echo "✅ Module loaded successfully!"
    echo ""
    echo "Module info:"
    lsmod | grep llama
    echo ""
    echo "Kernel messages:"
    dmesg | grep -E "(Llamux|llama|🦙)" | tail -10
else
    echo "❌ Failed to load module"
    echo "Error:"
    dmesg | tail -5
fi

echo ""
echo "Interactive shell. Type 'exit' to shutdown."
exec /bin/sh
EOF
chmod +x initramfs/init

# Create initramfs
cd initramfs
find . | cpio -o -H newc | gzip > ../initramfs.gz
cd ..

# Create run script
cat > run.sh << 'EOF'
#!/bin/bash
echo "🚀 Starting Llamux VM..."
echo "   - 2GB RAM"
echo "   - 2GB vmalloc"
echo "   - Llamux module auto-loaded"
echo ""

qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.gz \
    -m 2048 \
    -append "console=ttyS0 vmalloc=2048M quiet" \
    -nographic \
    -serial mon:stdio \
    -enable-kvm 2>/dev/null || \
qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.gz \
    -m 2048 \
    -append "console=ttyS0 vmalloc=2048M quiet" \
    -nographic \
    -serial mon:stdio
EOF
chmod +x run.sh

echo "✅ Instant VM ready!"
echo ""
echo "Run with: cd $VM_DIR && ./run.sh"
echo ""
echo "This will:"
echo "1. Boot a minimal Linux VM"
echo "2. With 2GB vmalloc space"
echo "3. Auto-load Llamux module"
echo "4. Give you a shell to test"
echo ""
echo "NO REBOOT OF YOUR HOST REQUIRED! 🎉"