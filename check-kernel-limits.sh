#!/bin/bash

echo "🦙 Current Kernel Memory Limits"
echo "=============================="
echo ""

echo "Kernel version:"
uname -r
echo ""

echo "Memory split:"
if grep -q CONFIG_VMSPLIT_3G=y /boot/config-$(uname -r) 2>/dev/null; then
    echo "  3G/1G (standard)"
elif grep -q CONFIG_VMSPLIT_2G=y /boot/config-$(uname -r) 2>/dev/null; then
    echo "  2G/2G (balanced)"
else
    echo "  Unknown"
fi
echo ""

echo "vmalloc info:"
grep -i vmalloc /proc/meminfo
echo ""

echo "Current boot parameters:"
cat /proc/cmdline
echo ""

echo "Kernel memory usage:"
echo "  Total RAM: $(free -h | awk '/^Mem:/{print $2}')"
echo "  Available: $(free -h | awk '/^Mem:/{print $7}')"
echo ""

echo "To load a 638MB model, we need:"
echo "  - At least 1GB contiguous vmalloc space"
echo "  - Custom kernel with CONFIG_VMSPLIT_2G"
echo "  - Boot param: vmalloc=4096M"