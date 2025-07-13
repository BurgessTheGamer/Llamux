#!/bin/bash
#
# Test script for Llamux LLM inference
# This script loads the module and tests token generation

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Find module
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_PATH="$SCRIPT_DIR/../kernel/llama_core/llama_core.ko"

echo "🦙 Llamux Inference Test"
echo "======================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

# Check if module exists
if [ ! -f "$MODULE_PATH" ]; then
    echo -e "${RED}Error: Module not found at $MODULE_PATH${NC}"
    echo "Build it with: make -C kernel/llama_core"
    exit 1
fi

# Function to cleanup
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    if lsmod | grep -q llama_core; then
        rmmod llama_core 2>/dev/null || true
    fi
}

# Set trap for cleanup
trap cleanup EXIT

# Remove existing module
if lsmod | grep -q llama_core; then
    echo "Removing existing module..."
    rmmod llama_core
    sleep 1
fi

# Clear kernel log
dmesg -C

# Load module
echo -e "${GREEN}Loading Llamux module...${NC}"
insmod "$MODULE_PATH"

# Check if loaded
if ! lsmod | grep -q llama_core; then
    echo -e "${RED}Failed to load module!${NC}"
    exit 1
fi

# Wait for initialization
echo "Waiting for initialization..."
sleep 2

# Check status
echo -e "\n${BLUE}Module Status:${NC}"
echo "=============="
cat /proc/llamux/status

# Test inference
echo -e "\n${BLUE}Testing Inference:${NC}"
echo "=================="

# Test 1: Simple prompt
echo -e "${YELLOW}Test 1: Simple greeting${NC}"
echo "Hello llama" > /proc/llamux/prompt
sleep 2
echo -e "${GREEN}Response:${NC}"
cat /proc/llamux/prompt

# Test 2: System question
echo -e "\n${YELLOW}Test 2: System question${NC}"
echo "What is kernel memory?" > /proc/llamux/prompt
sleep 2
echo -e "${GREEN}Response:${NC}"
cat /proc/llamux/prompt

# Test 3: Llamux specific
echo -e "\n${YELLOW}Test 3: Llamux question${NC}"
echo "Tell me about Llamux" > /proc/llamux/prompt
sleep 2
echo -e "${GREEN}Response:${NC}"
cat /proc/llamux/prompt

# Show kernel messages
echo -e "\n${BLUE}Kernel Messages:${NC}"
echo "================"
dmesg | grep -i llamux | tail -20

# Performance info
echo -e "\n${BLUE}Performance:${NC}"
echo "============"
TOKENS=$(dmesg | grep "Generated.*tokens" | tail -1 | grep -o "[0-9]* tokens" | awk '{print $1}')
if [ ! -z "$TOKENS" ]; then
    echo "Tokens generated: $TOKENS"
    echo "Approximate tokens/sec: $(($TOKENS / 2))"
fi

# Final status
echo -e "\n${BLUE}Final Status:${NC}"
echo "============="
cat /proc/llamux/status | grep -E "(Inference|Memory Used|Temperature)"

echo -e "\n${GREEN}✅ Inference test complete!${NC}"
echo ""
echo "🦙 Llamux is thinking in kernel space!"
echo ""
echo "Try your own prompts:"
echo "  echo \"Your question here\" > /proc/llamux/prompt"
echo "  cat /proc/llamux/prompt"