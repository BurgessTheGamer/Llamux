#!/bin/bash
set -e

echo "Creating Llamux Demo VM..."

# Clean up
rm -rf /root/Llamux/demo-vm
mkdir -p /root/Llamux/demo-vm
cd /root/Llamux/demo-vm

# Create minimal initramfs
mkdir -p initramfs/{bin,dev,proc,sys,lib/firmware/llamux}

# Copy busybox for basic utilities
cp /bin/busybox initramfs/bin/
cd initramfs/bin
for cmd in sh ls cat echo mount umount insmod lsmod dmesg sleep mkdir mknod; do
    ln -s busybox $cmd
done
cd ../..

# Copy kernel module
cp /root/Llamux/llamux/kernel/llama_core/llama_core.ko initramfs/

# Copy model
echo "Copying TinyLlama model..."
cp /lib/firmware/llamux/tinyllama.gguf initramfs/lib/firmware/llamux/

# Create init script
cat > initramfs/init << 'EOF'
#!/bin/sh

# Mount essential filesystems
/bin/mount -t proc none /proc
/bin/mount -t sysfs none /sys
/bin/mount -t devtmpfs none /dev

echo ""
echo "=== Llamux Demo VM ==="
echo "Kernel: $(uname -r)"
echo "Memory: $(cat /proc/meminfo | grep MemTotal)"
echo "Vmalloc: $(cat /proc/meminfo | grep VmallocTotal)"
echo ""

# Create device node if needed
/bin/mknod /dev/llama c 10 200 2>/dev/null || true

echo "Loading Llamux kernel module..."
/bin/insmod /llama_core.ko

if [ $? -eq 0 ]; then
    echo "✓ Module loaded successfully!"
    /bin/lsmod
    
    echo ""
    echo "Testing LLM..."
    echo "Hello, AI!" > /dev/llama
    echo "Response:"
    /bin/cat /dev/llama
else
    echo "✗ Failed to load module"
    /bin/dmesg | tail -20
fi

echo ""
echo "Starting shell..."
exec /bin/sh
EOF

chmod +x initramfs/init

# Build initramfs
find initramfs | cpio -o -H newc | gzip > initramfs.cpio.gz

# Create run script
cat > run.sh << 'EOF'
#!/bin/bash
qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.cpio.gz \
    -m 4G \
    -append "console=ttyS0 rdinit=/init vmalloc=3G quiet" \
    -nographic \
    -enable-kvm
EOF

chmod +x run.sh

echo "Demo VM created! Size: $(du -h initramfs.cpio.gz)"
echo "Run with: cd /root/Llamux/demo-vm && ./run.sh"