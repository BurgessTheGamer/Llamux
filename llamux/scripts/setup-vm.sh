#!/bin/bash
#
# Llamux VM Setup Script
# Creates and configures a QEMU VM for Llamux development

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VM_DIR="$PROJECT_ROOT/vm"

# VM Configuration
VM_NAME="llamux-dev"
VM_DISK="$VM_DIR/$VM_NAME.qcow2"
VM_SIZE="20G"
VM_RAM="4G"
VM_CPUS="4"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "🦙 Llamux VM Setup"
echo "=================="
echo ""

# Create VM directory
mkdir -p "$VM_DIR"

# Function to check dependencies
check_deps() {
    local missing=()
    
    for cmd in qemu-system-x86_64 qemu-img; do
        if ! command -v $cmd &> /dev/null; then
            missing+=($cmd)
        fi
    done
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}Missing dependencies:${NC} ${missing[*]}"
        echo "Install with:"
        echo "  Arch: sudo pacman -S qemu-full"
        echo "  Ubuntu: sudo apt install qemu-kvm qemu-utils"
        exit 1
    fi
}

# Check if VM already exists
if [ -f "$VM_DISK" ]; then
    echo -e "${YELLOW}VM disk already exists at:${NC} $VM_DISK"
    read -p "Delete and recreate? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Keeping existing VM. Use start-vm.sh to run it."
        exit 0
    fi
    rm -f "$VM_DISK"
fi

# Check dependencies
check_deps

# Download Arch Linux ISO if not present
ARCH_ISO="$VM_DIR/archlinux-x86_64.iso"
if [ ! -f "$ARCH_ISO" ]; then
    echo -e "${GREEN}Downloading Arch Linux ISO...${NC}"
    ARCH_MIRROR="https://mirror.rackspace.com/archlinux/iso/latest"
    
    # Get latest ISO filename
    ISO_NAME=$(curl -s "$ARCH_MIRROR/" | grep -o 'archlinux-[0-9.]*.iso' | head -1)
    
    if [ -z "$ISO_NAME" ]; then
        echo -e "${RED}Failed to find Arch ISO. Using direct link...${NC}"
        ISO_NAME="archlinux-x86_64.iso"
    fi
    
    wget -O "$ARCH_ISO" "$ARCH_MIRROR/$ISO_NAME" || {
        echo -e "${RED}Failed to download Arch ISO${NC}"
        echo "Please download manually from: https://archlinux.org/download/"
        exit 1
    }
fi

# Create VM disk
echo -e "${GREEN}Creating VM disk...${NC}"
qemu-img create -f qcow2 "$VM_DISK" "$VM_SIZE"

# Create start script
cat > "$VM_DIR/start-vm.sh" << 'EOF'
#!/bin/bash
# Start Llamux Development VM

VM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VM_DISK="$VM_DIR/llamux-dev.qcow2"

# Check if installing
if [ "$1" == "--install" ]; then
    CDROM="-cdrom $VM_DIR/archlinux-x86_64.iso -boot d"
else
    CDROM=""
fi

# Start VM
qemu-system-x86_64 \
    -enable-kvm \
    -m 4G \
    -cpu host,+avx2 \
    -smp 4 \
    -drive file="$VM_DISK",format=qcow2,if=virtio \
    $CDROM \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device virtio-net-pci,netdev=net0 \
    -display gtk \
    -monitor stdio \
    -name "Llamux Dev VM" \
    -machine q35,accel=kvm

EOF

chmod +x "$VM_DIR/start-vm.sh"

# Create SSH script
cat > "$VM_DIR/ssh-vm.sh" << 'EOF'
#!/bin/bash
# SSH into Llamux VM

ssh -p 2222 llamux@localhost "$@"
EOF

chmod +x "$VM_DIR/ssh-vm.sh"

# Create snapshot script
cat > "$VM_DIR/snapshot-vm.sh" << 'EOF'
#!/bin/bash
# Manage VM snapshots

VM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VM_DISK="$VM_DIR/llamux-dev.qcow2"

case "$1" in
    create)
        if [ -z "$2" ]; then
            echo "Usage: $0 create <snapshot-name>"
            exit 1
        fi
        qemu-img snapshot -c "$2" "$VM_DISK"
        echo "✅ Snapshot '$2' created"
        ;;
    list)
        qemu-img snapshot -l "$VM_DISK"
        ;;
    restore)
        if [ -z "$2" ]; then
            echo "Usage: $0 restore <snapshot-name>"
            exit 1
        fi
        qemu-img snapshot -a "$2" "$VM_DISK"
        echo "✅ Restored to snapshot '$2'"
        ;;
    delete)
        if [ -z "$2" ]; then
            echo "Usage: $0 delete <snapshot-name>"
            exit 1
        fi
        qemu-img snapshot -d "$2" "$VM_DISK"
        echo "✅ Snapshot '$2' deleted"
        ;;
    *)
        echo "Usage: $0 {create|list|restore|delete} [snapshot-name]"
        exit 1
        ;;
esac
EOF

chmod +x "$VM_DIR/snapshot-vm.sh"

# Create mount script for development
cat > "$VM_DIR/mount-project.sh" << 'EOF'
#!/bin/bash
# Mount project directory in VM using SSHFS

VM_USER="llamux"
VM_HOST="localhost"
VM_PORT="2222"
LOCAL_PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MOUNT_POINT="$HOME/llamux-host"

# Create mount point
mkdir -p "$MOUNT_POINT"

# Mount using SSHFS
sshfs -p $VM_PORT $VM_USER@$VM_HOST:/home/$VM_USER/llamux "$MOUNT_POINT" \
    -o follow_symlinks,ServerAliveInterval=15,ServerAliveCountMax=3

if [ $? -eq 0 ]; then
    echo "✅ Project mounted at: $MOUNT_POINT"
else
    echo "❌ Failed to mount project directory"
    echo "Make sure:"
    echo "  1. VM is running"
    echo "  2. SSH is configured"
    echo "  3. sshfs is installed"
fi
EOF

chmod +x "$VM_DIR/mount-project.sh"

# Print instructions
echo ""
echo -e "${GREEN}✅ VM setup complete!${NC}"
echo ""
echo "🦙 Quick Start Guide:"
echo "===================="
echo ""
echo "1. Start VM and install Arch Linux:"
echo "   $VM_DIR/start-vm.sh --install"
echo ""
echo "2. During installation, create a user 'llamux' with password"
echo ""
echo "3. After installation, start VM normally:"
echo "   $VM_DIR/start-vm.sh"
echo ""
echo "4. SSH into VM (after enabling SSH):"
echo "   $VM_DIR/ssh-vm.sh"
echo ""
echo "5. Manage snapshots:"
echo "   $VM_DIR/snapshot-vm.sh create base"
echo "   $VM_DIR/snapshot-vm.sh list"
echo "   $VM_DIR/snapshot-vm.sh restore base"
echo ""
echo "VM Configuration:"
echo "  Disk: $VM_DISK"
echo "  Size: $VM_SIZE"
echo "  RAM: $VM_RAM"
echo "  CPUs: $VM_CPUS"
echo "  SSH: localhost:2222"
echo ""
echo "🦙 Happy hacking!"