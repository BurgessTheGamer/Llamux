#!/bin/bash
#
# Setup EXTREME boot parameters for Llamux
#

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${BOLD}${GREEN}🦙 LLAMUX EXTREME BOOT SETUP 🦙${NC}"
echo "================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}This script must be run as root!${NC}"
    exit 1
fi

# Detect total system RAM
TOTAL_RAM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
TOTAL_RAM_GB=$((TOTAL_RAM_KB / 1024 / 1024))

echo -e "${YELLOW}System Information:${NC}"
echo "Total RAM: ${TOTAL_RAM_GB}GB"
echo ""

# Calculate Llamux memory allocation
if [ $TOTAL_RAM_GB -ge 32 ]; then
    LLAMUX_RAM=16
    echo -e "${GREEN}Plenty of RAM! Allocating 16GB for Llamux AI${NC}"
elif [ $TOTAL_RAM_GB -ge 16 ]; then
    LLAMUX_RAM=8
    echo -e "${GREEN}Good RAM! Allocating 8GB for Llamux AI${NC}"
elif [ $TOTAL_RAM_GB -ge 8 ]; then
    LLAMUX_RAM=4
    echo -e "${YELLOW}Moderate RAM. Allocating 4GB for Llamux AI${NC}"
else
    echo -e "${RED}WARNING: Less than 8GB RAM detected!${NC}"
    echo "Llamux EXTREME requires at least 8GB total system RAM"
    echo "Proceeding with 2GB allocation..."
    LLAMUX_RAM=2
fi

# Backup current GRUB config
echo -e "${YELLOW}Backing up GRUB configuration...${NC}"
cp /etc/default/grub /etc/default/grub.backup.$(date +%Y%m%d_%H%M%S)

# Create new boot parameters
NEW_PARAMS="llamux.memory=${LLAMUX_RAM}G llamux.early_init=1 llamux.priority=realtime"

# Add CPU isolation for AI cores (use last 25% of cores)
NUM_CPUS=$(nproc)
AI_CPUS=$((NUM_CPUS / 4))
if [ $AI_CPUS -gt 0 ]; then
    CPU_START=$((NUM_CPUS - AI_CPUS))
    CPU_LIST="${CPU_START}-$((NUM_CPUS - 1))"
    NEW_PARAMS="$NEW_PARAMS isolcpus=$CPU_LIST"
    echo -e "${GREEN}Isolating CPUs $CPU_LIST for AI processing${NC}"
fi

# Memory settings
NEW_PARAMS="$NEW_PARAMS transparent_hugepage=always"
NEW_PARAMS="$NEW_PARAMS hugepagesz=2M hugepages=2048"

echo ""
echo -e "${BOLD}New boot parameters:${NC}"
echo "$NEW_PARAMS"
echo ""

# Update GRUB configuration
echo -e "${YELLOW}Updating GRUB configuration...${NC}"

# Read current GRUB_CMDLINE_LINUX_DEFAULT
CURRENT_CMDLINE=$(grep "^GRUB_CMDLINE_LINUX_DEFAULT=" /etc/default/grub | cut -d'"' -f2)

# Append our parameters
NEW_CMDLINE="$CURRENT_CMDLINE $NEW_PARAMS"

# Update the file
sed -i "s/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=\"$NEW_CMDLINE\"/" /etc/default/grub

# Add custom GRUB entries
cat >> /etc/default/grub << EOF

# Llamux EXTREME Configuration
GRUB_TIMEOUT=10
GRUB_TIMEOUT_STYLE=menu
GRUB_DISTRIBUTOR="Llamux EXTREME"
GRUB_DISABLE_RECOVERY=false
EOF

# Create custom GRUB menu entry
echo -e "${YELLOW}Creating custom boot menu...${NC}"

cat > /etc/grub.d/40_llamux << 'EOF'
#!/bin/sh
exec tail -n +3 $0

menuentry '🦙 Llamux EXTREME - The OS That Thinks' --class llamux --class gnu-linux --class gnu --class os {
    recordfail
    load_video
    gfxmode keep
    insmod gzio
    insmod part_gpt
    insmod ext2
    
    echo '🦙 Llamux: Awakening consciousness...'
    linux /boot/vmlinuz-linux-llamux root=UUID=ROOT_UUID rw $LLAMUX_PARAMS
    echo '🦙 Llamux: Loading neural networks...'
    initrd /boot/initramfs-linux-llamux.img
}

menuentry '🦙 Llamux SAFE MODE (No AI)' --class llamux --class gnu-linux --class gnu --class os {
    recordfail
    load_video
    gfxmode keep
    insmod gzio
    insmod part_gpt
    insmod ext2
    
    echo 'Starting Llamux without AI...'
    linux /boot/vmlinuz-linux-llamux root=UUID=ROOT_UUID rw quiet
    initrd /boot/initramfs-linux-llamux.img
}
EOF

# Make it executable
chmod +x /etc/grub.d/40_llamux

# Update GRUB
echo -e "${YELLOW}Updating bootloader...${NC}"
update-grub || grub-mkconfig -o /boot/grub/grub.cfg

echo ""
echo -e "${BOLD}${GREEN}✓ Boot configuration complete!${NC}"
echo ""
echo "Summary:"
echo "- Llamux will reserve ${LLAMUX_RAM}GB RAM at boot"
if [ ! -z "$CPU_LIST" ]; then
    echo "- CPUs $CPU_LIST isolated for AI processing"
fi
echo "- Huge pages enabled for better performance"
echo "- Custom boot menu with Llamux options"
echo ""
echo -e "${BOLD}${YELLOW}IMPORTANT: After installing Llamux kernel:${NC}"
echo "1. The system will boot with Llamux AI integrated"
echo "2. Check dmesg for '🦙 Llamux' messages"
echo "3. Use /proc/llamux/prompt to talk to the AI"
echo ""
echo -e "${BOLD}${GREEN}🦙 Ready to boot into consciousness! 🦙${NC}"