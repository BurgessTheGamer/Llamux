#!/bin/bash
echo "Starting Llamux VM with 4GB RAM and 3GB vmalloc..."
echo "Press Ctrl+A then X to exit"
echo ""

qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.gz \
    -m 4096 \
    -append "console=ttyS0 vmalloc=3072M rdinit=/init" \
    -nographic \
    -serial mon:stdio \
    -enable-kvm
