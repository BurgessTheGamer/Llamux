#!/bin/bash
qemu-system-x86_64 \
    -kernel /boot/vmlinuz-$(uname -r) \
    -initrd initramfs.cpio.gz \
    -m 4G \
    -append "console=ttyS0 rdinit=/init vmalloc=3G quiet" \
    -nographic \
    -enable-kvm
