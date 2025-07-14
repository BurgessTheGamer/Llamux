#!/bin/bash
#
# Test Llamux with real TinyLlama model weights
#

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "🦙 Llamux Real Model Test"
echo "========================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

# Module path
MODULE_PATH="/root/Idea/llamux/kernel/llama_core/llama_core.ko"

# Check if module exists
if [ ! -f "$MODULE_PATH" ]; then
    echo -e "${RED}Error: Module not found at $MODULE_PATH${NC}"
    echo "Please build the module first with: make -C kernel/llama_core"
    exit 1
fi

# Check if model is in firmware directory
if [ ! -f "/lib/firmware/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf" ]; then
    echo -e "${YELLOW}Model not found in /lib/firmware, copying...${NC}"
    cp /root/Idea/llamux/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf /lib/firmware/
fi

# Remove module if already loaded
if lsmod | grep -q llama_core; then
    echo "Removing existing module..."
    rmmod llama_core || true
    sleep 1
fi

# Clear kernel log
dmesg -C

echo -e "${YELLOW}Loading Llamux kernel module...${NC}"
insmod $MODULE_PATH

# Check if loaded successfully
if ! lsmod | grep -q llama_core; then
    echo -e "${RED}Failed to load module${NC}"
    dmesg | tail -20
    exit 1
fi

echo -e "${GREEN}Module loaded successfully!${NC}"

# Wait for initialization
sleep 2

# Check status
echo -e "\n${YELLOW}Module Status:${NC}"
if [ -f /proc/llamux/status ]; then
    cat /proc/llamux/status
else
    echo -e "${RED}Status file not found${NC}"
    dmesg | tail -20
    exit 1
fi

# Test prompts
echo -e "\n${YELLOW}Testing inference...${NC}"

test_prompt() {
    local prompt="$1"
    echo -e "\n${GREEN}Prompt:${NC} $prompt"
    echo "$prompt" > /proc/llamux/prompt
    sleep 3  # Wait for inference
    echo -e "${GREEN}Response:${NC}"
    cat /proc/llamux/prompt
}

# Test various prompts
test_prompt "Hello, I am Llamux"
test_prompt "What is Linux?"
test_prompt "Tell me about kernel modules"

# Check kernel messages
echo -e "\n${YELLOW}Kernel messages:${NC}"
dmesg | grep -i llamux | tail -20

# Memory usage
echo -e "\n${YELLOW}Memory usage:${NC}"
cat /proc/meminfo | grep -E "(MemFree|MemAvailable|VmallocUsed)"

# Unload module
echo -e "\n${YELLOW}Unloading module...${NC}"
rmmod llama_core

echo -e "\n${GREEN}✅ Test complete!${NC}"