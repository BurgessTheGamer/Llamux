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
