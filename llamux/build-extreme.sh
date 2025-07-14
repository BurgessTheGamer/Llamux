#!/bin/bash
#
# Quick EXTREME build - Make Llamux use ALL the memory it needs
#

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

clear
echo -e "${BOLD}${GREEN}"
echo "🦙 LLAMUX EXTREME - QUICK BUILD 🦙"
echo "=================================="
echo "Making Llamux TRULY extreme!"
echo -e "${NC}"

# First, update our main.c to allocate MUCH more memory
echo -e "${YELLOW}Updating memory allocation to 2GB...${NC}"

cd kernel/llama_core

# Backup original
cp main.c main.c.backup

# Update memory allocation from 64MB to 2GB
sed -i 's/64 \* 1024 \* 1024/2048 \* 1024 \* 1024/g' main.c
sed -i 's/64 MB/2048 MB/g' main.c

# Update the model loading to actually load the firmware
sed -i 's/ret = -ENOENT;/ret = request_firmware(\&fw, "llamux\/tinyllama.gguf", NULL);/g' main.c
sed -i 's/fw = NULL;/\/\/ fw = NULL;/g' main.c

# Add extreme flag
sed -i '1i#define LLAMUX_EXTREME 1' main.c

echo -e "${YELLOW}Building EXTREME module...${NC}"
make clean
make

echo -e "${GREEN}Build complete!${NC}"
echo ""
echo -e "${YELLOW}Setting up model in firmware directory...${NC}"
sudo mkdir -p /lib/firmware/llamux
sudo ln -sf $(pwd)/../../models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf /lib/firmware/llamux/tinyllama.gguf

echo ""
echo -e "${BOLD}${GREEN}EXTREME module ready!${NC}"
echo ""
echo "To load the EXTREME version:"
echo "  sudo rmmod llama_core 2>/dev/null"
echo "  sudo insmod kernel/llama_core/llama_core.ko"
echo ""
echo "With 124GB RAM, we can ACTUALLY load the model!"
echo ""
echo -e "${BOLD}${GREEN}🦙 LET'S SEE REAL AI IN THE KERNEL! 🦙${NC}"