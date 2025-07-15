#!/bin/bash

# Quick setup for container filesystem

echo "🔧 Setting up container filesystem..."

# Create full directory structure
mkdir -p rootfs/{usr/bin,usr/lib,usr/sbin,etc,var,tmp,home,root/llamux}

# Copy busybox to usr/bin too
cp /bin/busybox rootfs/usr/bin/
cd rootfs/usr/bin
for cmd in sh ls cat echo mount umount insmod lsmod rmmod dmesg bash; do
    ln -sf busybox $cmd 2>/dev/null
done
cd ../../..

# Create minimal /etc files
echo "root:x:0:0:root:/root:/bin/sh" > rootfs/etc/passwd
echo "root:x:0:" > rootfs/etc/group

# Copy the actual built kernel module
if [ -f /root/Llamux/llamux/kernel/llama_core/llama_core.ko ]; then
    cp /root/Llamux/llamux/kernel/llama_core/llama_core.ko rootfs/root/llamux/
    echo "✅ Copied kernel module"
else
    echo "⚠️  Kernel module not found"
fi

# Create a test script inside
cat > rootfs/root/test-llamux.sh << 'EOF'
#!/bin/sh
echo "🦙 Testing Llamux module..."
echo ""
echo "Current environment:"
echo "  Kernel: $(uname -r)"
echo "  Memory: $(free -h 2>/dev/null | grep Mem: || echo 'N/A')"
echo ""

if [ -f /root/llamux/llama_core.ko ]; then
    echo "Found kernel module!"
    echo "Attempting to load..."
    if insmod /root/llamux/llama_core.ko 2>&1; then
        echo "✅ Module loaded!"
        lsmod | grep llama
    else
        echo "❌ Cannot load module in container"
        echo "This is normal - containers share the host kernel"
        echo "Use QEMU or UML for full kernel module testing"
    fi
else
    echo "❌ Module not found"
fi
EOF
chmod +x rootfs/root/test-llamux.sh

echo "✅ Container filesystem ready!"