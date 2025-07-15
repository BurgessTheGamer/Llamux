#!/bin/bash
set -e

echo "Setting up quick Llamux test environment..."

# First, let's modify the kernel module to allocate less memory
cd /root/Llamux/llamux/kernel/llama_core

# Backup original
cp llama_model.c llama_model.c.bak

# Reduce memory allocation from 768MB to 256MB for testing
sed -i 's/768 \* 1024 \* 1024/256 * 1024 * 1024/g' llama_model.c

# Rebuild module
make clean && make

echo "Module rebuilt with reduced memory requirements"

# Now test loading it on the host
echo "Testing module load on host system..."
sudo rmmod llama_core 2>/dev/null || true

echo "Current vmalloc info:"
grep Vmalloc /proc/meminfo

echo ""
echo "Loading module..."
sudo insmod llama_core.ko

if [ $? -eq 0 ]; then
    echo "✓ Module loaded successfully!"
    lsmod | grep llama_core
    
    # Check dmesg
    echo ""
    echo "Kernel messages:"
    dmesg | tail -10
    
    # Test device
    if [ -e /dev/llama ]; then
        echo ""
        echo "Testing LLM device..."
        echo "Hello from Llamux!" | sudo tee /dev/llama
        echo "Response:"
        sudo cat /dev/llama
    fi
    
    # Unload module
    echo ""
    echo "Unloading module..."
    sudo rmmod llama_core
else
    echo "✗ Failed to load module"
    dmesg | tail -20
fi

# Restore original
mv llama_model.c.bak llama_model.c