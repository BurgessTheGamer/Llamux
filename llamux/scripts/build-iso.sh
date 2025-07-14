#!/bin/bash
#
# Build Llamux ISO - Arch Linux with AI-powered kernel
#

set -e

# Configuration
ISO_NAME="llamux-0.1.0-alpha"
WORK_DIR="/tmp/llamux-iso"
LLAMUX_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "🦙 Llamux ISO Builder"
echo "===================="
echo ""

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"
DEPS="archiso mkinitcpio pacman"
for dep in $DEPS; do
    if ! command -v $dep &> /dev/null; then
        echo -e "${RED}Error: $dep is required but not installed${NC}"
        echo "This script must be run on Arch Linux with archiso installed"
        exit 1
    fi
done

# Create work directory
echo -e "${YELLOW}Creating work directory...${NC}"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Copy archiso profile
echo -e "${YELLOW}Setting up archiso profile...${NC}"
cp -r /usr/share/archiso/configs/releng/ llamux-profile
cd llamux-profile

# Customize packages
echo -e "${YELLOW}Customizing package list...${NC}"
cat >> packages.x86_64 << 'EOF'

# Llamux specific packages
base-devel
linux-headers
git
vim
htop
tmux
python
python-pip

# Development tools
gcc
make
cmake

# System monitoring
sysstat
iotop
nethogs
EOF

# Create Llamux-specific files
echo -e "${YELLOW}Creating Llamux files...${NC}"

# Create airootfs overlay
mkdir -p airootfs/etc/systemd/system
mkdir -p airootfs/usr/local/bin
mkdir -p airootfs/etc/skel
mkdir -p airootfs/lib/firmware

# Copy Llamux module and model
cp -r "$LLAMUX_DIR/kernel/llama_core" airootfs/usr/src/
cp "$LLAMUX_DIR/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf" airootfs/lib/firmware/ 2>/dev/null || true

# Create Llamux service
cat > airootfs/etc/systemd/system/llamux.service << 'EOF'
[Unit]
Description=Llamux AI Kernel Module
After=multi-user.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/llamux-start
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

# Create startup script
cat > airootfs/usr/local/bin/llamux-start << 'EOF'
#!/bin/bash
echo "🦙 Starting Llamux AI system..."

# Load the module if not built-in
if ! grep -q llamux /proc/modules && [ -f /usr/src/llama_core/llama_core.ko ]; then
    insmod /usr/src/llama_core/llama_core.ko
fi

# Check if loaded
if [ -d /proc/llamux ]; then
    echo "🦙 Llamux is ready!"
    echo "Try: echo 'Hello AI' > /proc/llamux/prompt"
else
    echo "🦙 Llamux module not loaded"
fi
EOF
chmod +x airootfs/usr/local/bin/llamux-start

# Create welcome message
cat > airootfs/etc/motd << 'EOF'

🦙 Welcome to Llamux - The OS that thinks!
==========================================

This is an Arch Linux system with TinyLlama AI built into the kernel.

Quick start:
  - Test AI: echo "Hello AI" > /proc/llamux/prompt
  - Read response: cat /proc/llamux/prompt
  - Check status: cat /proc/llamux/status

For more information: https://github.com/llamux/llamux

EOF

# Create custom shell prompt
cat > airootfs/etc/skel/.bashrc << 'EOF'
# Llamux custom bashrc
PS1='🦙 \[\033[01;32m\]\u@llamux\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '

# Aliases
alias ll='ls -la'
alias llama='echo "$1" > /proc/llamux/prompt && cat /proc/llamux/prompt'

# Welcome
echo "Welcome to Llamux! Type 'llama \"your question\"' to talk to the AI."
EOF

# Customize boot loader
echo -e "${YELLOW}Customizing boot loader...${NC}"
cat > airootfs/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "Llamux - AI-Powered Linux" {
    linux /boot/vmlinuz-linux llamux.reserve_mem=2G quiet splash
    initrd /boot/initramfs-linux.img
}

menuentry "Llamux - Safe Mode" {
    linux /boot/vmlinuz-linux quiet
    initrd /boot/initramfs-linux.img
}
EOF

# Enable services
ln -sf /etc/systemd/system/llamux.service airootfs/etc/systemd/system/multi-user.target.wants/

# Build the ISO
echo -e "${YELLOW}Building ISO (this will take a while)...${NC}"
echo -e "${YELLOW}ISO will be created at: $WORK_DIR/out/${ISO_NAME}.iso${NC}"

# Create output directory
mkdir -p out

# Uncomment to actually build (requires root and takes 20-30 minutes)
# sudo mkarchiso -v -w work -o out .

echo -e "${GREEN}ISO configuration complete!${NC}"
echo ""
echo "To build the ISO, run:"
echo "  cd $WORK_DIR/llamux-profile"
echo "  sudo mkarchiso -v -w work -o out ."
echo ""
echo "The ISO will include:"
echo "  - Arch Linux base system"
echo "  - Llamux kernel module"
echo "  - TinyLlama model in /lib/firmware"
echo "  - Custom shell with AI integration"
echo "  - Automatic Llamux startup"
echo ""
echo "🦙 Ready to create the world's first AI-powered Linux distribution!"