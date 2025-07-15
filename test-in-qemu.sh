#!/bin/bash

# Test Llamux kernel in QEMU without rebooting the host

set -e

echo "🦙 Llamux QEMU Test Environment"
echo "==============================="
echo "Test the custom kernel without rebooting!"
echo ""

KERNEL_DIR="/root/Llamux/kernel-build"
QEMU_DIR="/root/Llamux/qemu-test"
DEBIAN_IMG="debian-12-minimal.qcow2"

# Check if kernel is built
if ! find $KERNEL_DIR -name "linux-image-*-llamux*.deb" | grep -q deb; then
    echo "❌ No Llamux kernel found. Build it first!"
    exit 1
fi

# Create QEMU test directory
mkdir -p $QEMU_DIR
cd $QEMU_DIR

# Step 1: Create minimal Debian image if not exists
if [ ! -f "$DEBIAN_IMG" ]; then
    echo "📦 Creating minimal Debian 12 image..."
    
    # Create 10GB disk image
    qemu-img create -f qcow2 $DEBIAN_IMG 10G
    
    # Download Debian netinst
    if [ ! -f debian-12-netinst.iso ]; then
        echo "Downloading Debian 12 installer..."
        wget -q --show-progress https://cdimage.debian.org/debian-cd/current/amd64/iso-cd/debian-12.9.0-amd64-netinst.iso -O debian-12-netinst.iso
    fi
    
    echo "❗ Manual step required:"
    echo "   1. Run QEMU to install Debian:"
    echo "      qemu-system-x86_64 -m 4G -hda $DEBIAN_IMG -cdrom debian-12-netinst.iso -boot d"
    echo "   2. Do minimal install (no desktop)"
    echo "   3. Shutdown when complete"
    echo "   4. Re-run this script"
    exit 0
fi

# Step 2: Copy kernel and module to image
echo "📦 Preparing test environment..."

# Mount the image
modprobe nbd max_part=8
qemu-nbd --connect=/dev/nbd0 $DEBIAN_IMG
sleep 2

# Create mount point
mkdir -p /mnt/qemu-test

# Mount root partition (usually nbd0p1)
if mount /dev/nbd0p1 /mnt/qemu-test 2>/dev/null || mount /dev/nbd0p2 /mnt/qemu-test 2>/dev/null; then
    echo "✅ Mounted test image"
    
    # Copy kernel packages
    echo "📦 Copying kernel packages..."
    mkdir -p /mnt/qemu-test/root/llamux-kernel
    cp $KERNEL_DIR/*.deb /mnt/qemu-test/root/llamux-kernel/ 2>/dev/null || true
    cp $KERNEL_DIR/linux-source-6.1/../*.deb /mnt/qemu-test/root/llamux-kernel/ 2>/dev/null || true
    
    # Copy Llamux module and model
    echo "📦 Copying Llamux module and model..."
    cp -r /root/Llamux/llamux /mnt/qemu-test/root/
    
    # Create install script
    cat > /mnt/qemu-test/root/install-llamux.sh << 'EOF'
#!/bin/bash
echo "🦙 Installing Llamux kernel..."
cd /root/llamux-kernel
dpkg -i linux-image-*-llamux*.deb
dpkg -i linux-headers-*-llamux*.deb || true

echo "🦙 Setting up Llamux model..."
mkdir -p /lib/firmware/llamux
cp /root/llamux/models/*.gguf /lib/firmware/llamux/tinyllama.gguf

echo "🦙 Updating GRUB..."
sed -i 's/GRUB_CMDLINE_LINUX="\(.*\)"/GRUB_CMDLINE_LINUX="\1 vmalloc=4096M"/' /etc/default/grub
update-grub

echo "✅ Llamux kernel installed!"
echo "Reboot to use the new kernel"
EOF
    chmod +x /mnt/qemu-test/root/install-llamux.sh
    
    # Unmount
    umount /mnt/qemu-test
    echo "✅ Test environment prepared"
else
    echo "❌ Failed to mount image"
    qemu-nbd --disconnect /dev/nbd0
    exit 1
fi

# Disconnect NBD
qemu-nbd --disconnect /dev/nbd0

# Step 3: Create run script
cat > run-llamux-test.sh << 'EOF'
#!/bin/bash
echo "🚀 Starting Llamux test VM..."
echo "   - 4GB RAM allocated"
echo "   - Serial console enabled"
echo "   - Use Ctrl-A X to exit"
echo ""

qemu-system-x86_64 \
    -m 4G \
    -smp 4 \
    -hda debian-12-minimal.qcow2 \
    -enable-kvm \
    -nographic \
    -serial mon:stdio \
    -append "console=ttyS0"
EOF
chmod +x run-llamux-test.sh

echo ""
echo "✅ QEMU test environment ready!"
echo ""
echo "To test Llamux:"
echo "1. Start VM: ./run-llamux-test.sh"
echo "2. Login and run: /root/install-llamux.sh"
echo "3. Reboot VM and select Llamux kernel"
echo "4. Test module: cd /root/llamux && insmod kernel/llama_core/llama_core.ko"
echo ""
echo "This lets you test everything without touching your host system! 🎉"