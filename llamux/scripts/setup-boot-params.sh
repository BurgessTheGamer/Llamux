#!/bin/bash
#
# Setup Llamux boot parameters
# This script helps configure GRUB to reserve memory for Llamux

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "🦙 Llamux Boot Parameter Setup"
echo "=============================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

# Detect bootloader
if [ -f /etc/default/grub ]; then
    BOOTLOADER="grub"
    GRUB_CONFIG="/etc/default/grub"
elif [ -f /boot/loader/loader.conf ]; then
    BOOTLOADER="systemd-boot"
    LOADER_CONFIG="/boot/loader/loader.conf"
else
    echo -e "${RED}Error: Could not detect bootloader (GRUB or systemd-boot)${NC}"
    exit 1
fi

echo "Detected bootloader: $BOOTLOADER"

# Ask for memory size
echo ""
echo "How much memory should Llamux reserve for the LLM?"
echo "Recommendations:"
echo "  - TinyLlama Q4_K_M: 1G (minimum)"
echo "  - TinyLlama with headroom: 2G (recommended)"
echo "  - Future larger models: 4G"
echo ""
read -p "Enter size (e.g., 2G, 2048M): " MEM_SIZE

# Validate input
if ! echo "$MEM_SIZE" | grep -qE '^[0-9]+[GMK]?$'; then
    echo -e "${RED}Error: Invalid memory size format${NC}"
    exit 1
fi

# Create backup
if [ "$BOOTLOADER" = "grub" ]; then
    echo "Creating backup of $GRUB_CONFIG..."
    cp "$GRUB_CONFIG" "$GRUB_CONFIG.backup.$(date +%Y%m%d_%H%M%S)"
    
    # Check if llamux_mem already exists
    if grep -q "llamux_mem=" "$GRUB_CONFIG"; then
        echo -e "${YELLOW}Warning: llamux_mem parameter already exists${NC}"
        read -p "Update existing parameter? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 0
        fi
        # Remove existing parameter
        sed -i 's/llamux_mem=[^ ]*//' "$GRUB_CONFIG"
    fi
    
    # Add Llamux parameters
    echo "Adding Llamux boot parameters..."
    if grep -q "^GRUB_CMDLINE_LINUX=" "$GRUB_CONFIG"; then
        # Append to existing line
        sed -i "/^GRUB_CMDLINE_LINUX=/ s/\"\$/ llamux_mem=$MEM_SIZE\"/" "$GRUB_CONFIG"
    else
        # Add new line
        echo "GRUB_CMDLINE_LINUX=\"llamux_mem=$MEM_SIZE\"" >> "$GRUB_CONFIG"
    fi
    
    echo "Updated GRUB configuration:"
    grep "^GRUB_CMDLINE_LINUX=" "$GRUB_CONFIG"
    
    # Update GRUB
    echo ""
    echo "Updating GRUB..."
    if command -v update-grub &> /dev/null; then
        update-grub
    elif command -v grub-mkconfig &> /dev/null; then
        grub-mkconfig -o /boot/grub/grub.cfg
    elif command -v grub2-mkconfig &> /dev/null; then
        grub2-mkconfig -o /boot/grub2/grub.cfg
    else
        echo -e "${RED}Error: Could not find GRUB update command${NC}"
        echo "Please manually run your distribution's GRUB update command"
    fi
    
elif [ "$BOOTLOADER" = "systemd-boot" ]; then
    # For systemd-boot, we need to edit the kernel command line in entries
    ENTRIES_DIR="/boot/loader/entries"
    
    if [ ! -d "$ENTRIES_DIR" ]; then
        echo -e "${RED}Error: systemd-boot entries directory not found${NC}"
        exit 1
    fi
    
    echo "Updating systemd-boot entries..."
    for entry in "$ENTRIES_DIR"/*.conf; do
        if [ -f "$entry" ]; then
            echo "Updating $entry..."
            cp "$entry" "$entry.backup.$(date +%Y%m%d_%H%M%S)"
            
            if grep -q "llamux_mem=" "$entry"; then
                sed -i 's/llamux_mem=[^ ]*//' "$entry"
            fi
            
            # Add to options line
            sed -i "/^options/ s/$/ llamux_mem=$MEM_SIZE/" "$entry"
        fi
    done
fi

# Show next steps
echo ""
echo -e "${GREEN}✅ Boot parameters configured!${NC}"
echo ""
echo "🦙 Next steps:"
echo "1. Reboot your system"
echo "2. Verify with: cat /proc/cmdline | grep llamux_mem"
echo "3. Load Llamux module: sudo insmod /path/to/llama_core.ko"
echo "4. Check memory: cat /proc/llamux/status"
echo ""
echo "To see current kernel parameters without rebooting:"
echo "  cat /proc/cmdline"
echo ""

# Optional: Show what memmap would look like
echo -e "${YELLOW}Advanced: Direct memory mapping${NC}"
echo "For even better performance, you can use memmap to reserve physical memory:"
echo "  memmap=$MEM_SIZE\\\$0x100000000"
echo "This reserves memory starting at 4GB physical address."
echo ""
echo "🦙 Happy hacking!"