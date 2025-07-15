#!/bin/bash
set -e

echo "Setting up Llamux VM with real TinyLlama model..."

cd /root/Llamux/instant-vm

# Create initramfs structure
mkdir -p initramfs/{bin,sbin,lib,lib64,proc,sys,dev,tmp,etc,root,lib/firmware/llamux}

# Copy essential binaries
cp /bin/{bash,ls,cat,echo,mount,umount,insmod,lsmod,dmesg,sleep} initramfs/bin/ || true
cp /sbin/{init,modprobe} initramfs/sbin/ || true

# Copy libraries
ldd /bin/bash | grep -o '/lib.*\.[0-9]' | xargs -I {} cp {} initramfs/lib64/ || true
cp /lib/x86_64-linux-gnu/libtinfo.so.6 initramfs/lib64/ || true
cp -a /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 initramfs/lib64/ || true

# Copy kernel module
cp /root/Llamux/llamux/kernel/llama_core/llama_core.ko initramfs/

# Copy real TinyLlama model
echo "Copying TinyLlama model (638MB)..."
cp /lib/firmware/llamux/tinyllama.gguf initramfs/lib/firmware/llamux/

# Create init script
cat > initramfs/init << 'EOF'
#!/bin/bash
export PATH=/bin:/sbin

# Mount essential filesystems
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo "Llamux VM booted with vmalloc=$(cat /proc/cmdline | grep -o 'vmalloc=[0-9]*M')"
echo "Available memory: $(free -h)"

# Show vmalloc info
echo "VmallocTotal: $(grep VmallocTotal /proc/meminfo)"
echo "VmallocUsed: $(grep VmallocUsed /proc/meminfo)"

echo ""
echo "Loading Llamux kernel module..."
insmod /llama_core.ko

if [ $? -eq 0 ]; then
    echo "Module loaded successfully!"
    lsmod | grep llama_core
    
    # Check if device was created
    if [ -e /dev/llama ]; then
        echo ""
        echo "LLM device created at /dev/llama"
        echo "You can now interact with the LLM using:"
        echo "  echo 'Your prompt here' > /dev/llama"
        echo "  cat /dev/llama"
    fi
else
    echo "Failed to load module. Check dmesg for details."
fi

echo ""
echo "Type 'dmesg' to see kernel messages"
echo "Type 'echo \"Hello AI\" > /dev/llama && cat /dev/llama' to test the LLM"

# Start shell
exec /bin/bash
EOF

chmod +x initramfs/init

# Create device nodes
mknod initramfs/dev/console c 5 1
mknod initramfs/dev/null c 1 3
mknod initramfs/dev/tty c 5 0

# Build initramfs
echo "Building initramfs..."
find initramfs -print0 | cpio --null -o --format=newc | gzip -9 > initramfs.gz

# Create run script
cat > run.sh << 'EOF'
#!/bin/bash
echo "Starting Llamux VM with 4GB RAM and 3GB vmalloc..."
echo "Press Ctrl+A then X to exit"
echo ""

qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.gz \
    -m 4096 \
    -append "console=ttyS0 vmalloc=3072M quiet" \
    -nographic \
    -serial mon:stdio \
    -enable-kvm
EOF

chmod +x run.sh

echo "Setup complete! VM is ready to run."
echo "Initramfs size: $(du -h initramfs.gz | cut -f1)"