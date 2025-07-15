#!/bin/bash

# Quick setup for testing Llamux with current kernel + vmalloc parameter

echo "🦙 Llamux Quick Setup"
echo "===================="
echo ""
echo "This will set up Llamux to work with your current kernel"
echo "by adding vmalloc=4096M boot parameter"
echo ""

# Check current vmalloc
echo "Current vmalloc info:"
grep -i vmalloc /proc/meminfo
echo ""

# Check if we already have vmalloc in cmdline
if grep -q "vmalloc=" /proc/cmdline; then
    echo "✅ vmalloc parameter already set in current boot:"
    grep -o "vmalloc=[0-9]*M" /proc/cmdline
else
    echo "❌ No vmalloc parameter in current boot"
fi
echo ""

echo "Choose an option:"
echo "1) Set up boot parameter (requires reboot)"
echo "2) Try loading module anyway (might work with smaller allocation)"
echo "3) Test in Docker with privileged mode"
echo ""
read -p "Choice (1-3): " choice

case $choice in
    1)
        echo ""
        echo "Setting up boot parameters..."
        
        # Backup grub config
        sudo cp /etc/default/grub /etc/default/grub.backup.$(date +%Y%m%d)
        
        # Add vmalloc parameter
        if ! grep -q "vmalloc=" /etc/default/grub; then
            sudo sed -i 's/GRUB_CMDLINE_LINUX="\(.*\)"/GRUB_CMDLINE_LINUX="\1 vmalloc=4096M"/' /etc/default/grub
            echo "✅ Added vmalloc=4096M to GRUB"
        else
            echo "⚠️  vmalloc already in GRUB config"
        fi
        
        # Update grub
        sudo update-grub
        
        echo ""
        echo "✅ Setup complete!"
        echo ""
        echo "Next steps:"
        echo "1. Reboot your system"
        echo "2. After reboot, run: $0 and choose option 2"
        ;;
        
    2)
        echo ""
        echo "Attempting to load module..."
        
        # First, let's try with a smaller memory allocation
        cd /root/Llamux/llamux/kernel/llama_core
        
        # Check if module exists
        if [ ! -f llama_core.ko ]; then
            echo "Building module first..."
            make clean && make
        fi
        
        # Remove if already loaded
        sudo rmmod llama_core 2>/dev/null || true
        
        # Try to load
        echo "Loading module..."
        if sudo insmod llama_core.ko; then
            echo "✅ Module loaded!"
            echo ""
            echo "Checking status:"
            lsmod | grep llama_core
            echo ""
            echo "Recent kernel messages:"
            dmesg | tail -20 | grep -E "(Llamux|llama|🦙)"
        else
            echo "❌ Failed to load module"
            echo ""
            echo "Error messages:"
            dmesg | tail -10
            echo ""
            echo "Try option 1 to set up proper vmalloc"
        fi
        ;;
        
    3)
        echo ""
        echo "Setting up Docker test..."
        
        # Create Dockerfile for testing
        cat > /root/Llamux/Dockerfile.test << 'EOF'
FROM debian:12

RUN apt-get update && apt-get install -y \
    build-essential \
    linux-headers-amd64 \
    kmod \
    wget \
    && rm -rf /var/lib/apt/lists/*

COPY llamux /llamux
WORKDIR /llamux

RUN echo '#!/bin/bash\n\
echo "🦙 Llamux Docker Test"\n\
echo "==================="\n\
echo "Note: This is limited testing only"\n\
echo "Full kernel module testing requires real kernel"\n\
cd /llamux/kernel/llama_core\n\
make clean && make\n\
echo "Module built. Cannot load in container."' > /test.sh && chmod +x /test.sh

CMD ["/test.sh"]
EOF

        echo "Building Docker image..."
        cd /root/Llamux
        docker build -f Dockerfile.test -t llamux-test .
        
        echo ""
        echo "✅ Docker image built!"
        echo "Run with: docker run -it --rm llamux-test"
        echo ""
        echo "Note: Docker can't load kernel modules, but can test building"
        ;;
        
    *)
        echo "Invalid choice"
        ;;
esac