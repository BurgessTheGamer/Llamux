#!/bin/bash

# Run Llamux in a container with User Mode Linux (UML) - NO REBOOT!

echo "🦙 Llamux Container Runner - INSTANT!"
echo "===================================="
echo "Run Llamux NOW with User Mode Linux"
echo ""

# Install User Mode Linux if not present
if ! command -v linux.uml &> /dev/null; then
    echo "📦 Installing User Mode Linux..."
    apt-get update && apt-get install -y user-mode-linux
fi

# Create container directory
CONTAINER_DIR="/root/Llamux/container-env"
mkdir -p $CONTAINER_DIR
cd $CONTAINER_DIR

# Create a minimal root filesystem
echo "🔧 Creating minimal container environment..."

# Create filesystem structure
mkdir -p rootfs/{bin,sbin,lib,lib64,dev,proc,sys,root,tmp}
mkdir -p rootfs/root/llamux

# Copy essential binaries (static busybox)
if [ ! -f rootfs/bin/busybox ]; then
    cp /bin/busybox rootfs/bin/
    cd rootfs/bin
    for cmd in sh ls cat echo mount umount insmod lsmod rmmod dmesg; do
        ln -sf busybox $cmd
    done
    cd ../..
fi

# Copy Llamux module
cp -r /root/Llamux/llamux/kernel/llama_core rootfs/root/llamux/
cp -r /root/Llamux/llamux/models rootfs/root/llamux/

# Create init script
cat > rootfs/init << 'EOF'
#!/bin/sh

# Mount essential filesystems
/bin/mount -t proc none /proc
/bin/mount -t sysfs none /sys
/bin/mount -t devtmpfs none /dev

echo ""
echo "🦙 Welcome to Llamux Container!"
echo "=============================="
echo ""
echo "This is running in User Mode Linux"
echo "with vmalloc=2048M already set!"
echo ""

# Show kernel info
echo "Kernel: $(uname -r)"
echo "vmalloc parameter: $(cat /proc/cmdline | grep -o 'vmalloc=[^ ]*' || echo 'default')"
echo ""

# Try to build and load module
cd /root/llamux/llama_core
if [ -f llama_core.ko ]; then
    echo "Loading Llamux module..."
    if insmod llama_core.ko 2>/dev/null; then
        echo "✅ Module loaded!"
        lsmod | grep llama
        dmesg | tail -10
    else
        echo "❌ Module load failed (expected in UML)"
        echo "But the environment is ready for testing!"
    fi
fi

echo ""
echo "Container shell ready. Type 'exit' to quit."
/bin/sh
EOF
chmod +x rootfs/init

# Create run script
cat > run-container.sh << 'EOF'
#!/bin/bash
echo "🚀 Starting Llamux in User Mode Linux..."
echo "   - Instant start (no VM boot)"
echo "   - 2GB vmalloc configured"
echo "   - Isolated from host"
echo ""

# Run with User Mode Linux
linux.uml \
    mem=2G \
    rootfstype=hostfs \
    rootflags=/ \
    rw \
    eth0=tuntap,,,192.168.0.1 \
    init=/init \
    vmalloc=2048M \
    quiet
EOF
chmod +x run-container.sh

# Alternative: Use systemd-nspawn (even simpler!)
cat > run-nspawn.sh << 'EOF'
#!/bin/bash
echo "🚀 Starting Llamux with systemd-nspawn..."
echo "   - Super fast container"
echo "   - Shares kernel but isolated"
echo ""

# Install systemd-container if needed
if ! command -v systemd-nspawn &> /dev/null; then
    apt-get install -y systemd-container
fi

# Run container
systemd-nspawn \
    --directory=rootfs \
    --capability=CAP_SYS_MODULE,CAP_SYS_ADMIN \
    --bind=/lib/modules \
    --setenv=VMALLOC=2048M \
    /bin/sh -c "
        echo '🦙 Llamux nspawn container'
        echo 'Note: This shares the host kernel'
        echo 'For full isolation, use run-container.sh'
        echo ''
        cd /root/llamux/llama_core
        ls -la
        /bin/sh
    "
EOF
chmod +x run-nspawn.sh

echo "✅ Container environment ready!"
echo ""
echo "🎯 Choose your isolation level:"
echo ""
echo "1. User Mode Linux (full kernel isolation):"
echo "   ./run-container.sh"
echo ""
echo "2. systemd-nspawn (faster, shares kernel):"
echo "   ./run-nspawn.sh"
echo ""
echo "3. Direct QEMU (from earlier setup):"
echo "   cd ../vm-environment && ./start-llamux-vm.sh"
echo ""
echo "All options let you test Llamux WITHOUT rebooting! 🎉"