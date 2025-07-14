#!/bin/bash
# Simple test script for Llamux

echo "🦙 Llamux Simple Test"
echo "===================="

# Clean up any existing module
sudo rmmod llama_core 2>/dev/null || true
sudo rm -rf /proc/llamux 2>/dev/null || true

# Wait a bit
sleep 2

# Load the module
echo "Loading module..."
sudo insmod kernel/llama_core/llama_core.ko

# Check if loaded
if lsmod | grep -q llama_core; then
    echo "✅ Module loaded successfully!"
else
    echo "❌ Module failed to load"
    exit 1
fi

# Check dmesg
echo -e "\nKernel messages:"
dmesg | tail -20 | grep -i llamux || echo "No Llamux messages found"

# Check proc interface
echo -e "\nChecking /proc/llamux:"
ls -la /proc/llamux/ 2>/dev/null || echo "No /proc/llamux directory"

# Try to interact
echo -e "\nTrying to send a prompt..."
echo "Hello, AI!" | sudo tee /proc/llamux/prompt 2>&1

# Read response
echo -e "\nReading response..."
sudo cat /proc/llamux/prompt 2>&1 || echo "Failed to read response"

# Check status
echo -e "\nChecking status..."
sudo cat /proc/llamux/status 2>&1 || echo "Failed to read status"

echo -e "\n🦙 Test complete!"