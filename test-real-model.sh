#!/bin/bash

# Test script for loading real TinyLlama model

echo "🦙 Llamux Real Model Test"
echo "========================"

# Check current vmalloc usage
echo "Current vmalloc info:"
sudo cat /proc/meminfo | grep -i vmalloc

echo ""
echo "Loading module..."
sudo dmesg -C
sudo insmod llamux/kernel/llama_core/llama_core.ko

if lsmod | grep -q llama_core; then
    echo "✅ Module loaded!"
    
    # Check dmesg
    echo ""
    echo "Kernel messages:"
    sudo dmesg | grep -E "(Llamux|llama|GGML)" | tail -20
    
    # Test inference
    echo ""
    echo "Testing inference..."
    echo "Hello, Llama!" | sudo tee /proc/llamux/prompt
    
    sleep 2
    
    if [ -f /proc/llamux/response ]; then
        echo "Response:"
        cat /proc/llamux/response
    fi
else
    echo "❌ Failed to load module"
    echo ""
    echo "Error messages:"
    sudo dmesg | tail -10
fi