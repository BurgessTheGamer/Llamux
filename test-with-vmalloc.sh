#!/bin/bash

# Test Llamux module with increased vmalloc boot parameter

echo "🦙 Llamux vmalloc Test"
echo "====================="
echo ""
echo "This script will:"
echo "1. Update GRUB to add vmalloc=4096M"
echo "2. Require a reboot"
echo "3. Then load the Llamux module"
echo ""

if [ "$1" == "setup" ]; then
    echo "Setting up boot parameters..."
    
    # Backup current grub config
    cp /etc/default/grub /etc/default/grub.backup
    
    # Add vmalloc parameter
    if ! grep -q "vmalloc=" /etc/default/grub; then
        sed -i 's/GRUB_CMDLINE_LINUX="\(.*\)"/GRUB_CMDLINE_LINUX="\1 vmalloc=4096M"/' /etc/default/grub
        echo "✅ Added vmalloc=4096M to boot parameters"
    else
        echo "⚠️  vmalloc parameter already set"
    fi
    
    # Update grub
    update-grub
    
    echo ""
    echo "✅ Setup complete!"
    echo "Please reboot and run: $0 test"
    
elif [ "$1" == "test" ]; then
    echo "Testing with current vmalloc settings..."
    
    # Check current vmalloc
    echo "Current kernel parameters:"
    cat /proc/cmdline
    echo ""
    
    echo "vmalloc info:"
    grep -i vmalloc /proc/meminfo
    echo ""
    
    # Load module
    cd /root/Llamux
    if [ -f llamux/kernel/llama_core/llama_core.ko ]; then
        echo "Loading Llamux module..."
        insmod llamux/kernel/llama_core/llama_core.ko
        
        if lsmod | grep -q llama_core; then
            echo "✅ Module loaded!"
            dmesg | tail -20
        else
            echo "❌ Failed to load module"
            dmesg | tail -10
        fi
    else
        echo "❌ Module not found. Build it first!"
    fi
    
else
    echo "Usage:"
    echo "  $0 setup  - Configure boot parameters"
    echo "  $0 test   - Test module loading"
fi