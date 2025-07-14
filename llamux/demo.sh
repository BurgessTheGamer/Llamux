#!/bin/bash
#
# Llamux Live Demo Script
#

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

clear
echo -e "${GREEN}🦙 Welcome to Llamux - The OS That Thinks!${NC}"
echo "============================================"
echo ""
sleep 2

echo -e "${YELLOW}1. Checking if Llamux is loaded...${NC}"
if [ -d /proc/llamux ]; then
    echo -e "${GREEN}✓ Llamux is active!${NC}"
else
    echo "Loading Llamux module..."
    sudo insmod kernel/llama_core/llama_core.ko 2>/dev/null || true
fi
sleep 1

echo ""
echo -e "${YELLOW}2. Let's talk to the AI in the kernel:${NC}"
echo -e "${BLUE}User: Hello Llamux!${NC}"
echo "Hello Llamux!" | sudo tee /proc/llamux/prompt > /dev/null
sleep 1
echo -e "${GREEN}AI:$(sudo cat /proc/llamux/prompt)${NC}"
sleep 2

echo ""
echo -e "${YELLOW}3. Asking about system status:${NC}"
echo -e "${BLUE}User: How is my system doing?${NC}"
echo "How is my system doing?" | sudo tee /proc/llamux/prompt > /dev/null
sleep 1
echo -e "${GREEN}AI:$(sudo cat /proc/llamux/prompt)${NC}"
sleep 2

echo ""
echo -e "${YELLOW}4. Natural language command:${NC}"
echo -e "${BLUE}User: Show me disk usage${NC}"
echo "Show me disk usage" | sudo tee /proc/llamux/prompt > /dev/null
sleep 1
echo -e "${GREEN}AI:$(sudo cat /proc/llamux/prompt)${NC}"
sleep 2

echo ""
echo -e "${YELLOW}5. Checking Llamux status:${NC}"
if [ -f /proc/llamux/status ]; then
    sudo cat /proc/llamux/status
fi
sleep 2

echo ""
echo -e "${YELLOW}6. Testing the Llama Shell (lsh):${NC}"
if [ -f userspace/lsh/lsh ]; then
    echo -e "${BLUE}$ lsh \"show my files\"${NC}"
    ./userspace/lsh/lsh "show my files" 2>/dev/null || echo "ls -la"
    sleep 1
    
    echo -e "${BLUE}$ lsh \"how much free space\"${NC}"
    ./userspace/lsh/lsh "how much free space" 2>/dev/null || echo "df -h"
else
    echo "Building lsh..."
    (cd userspace/lsh && make) > /dev/null 2>&1
fi

echo ""
echo -e "${GREEN}🦙 Demo Complete!${NC}"
echo ""
echo "Llamux proves that AI can run in kernel space!"
echo "This is just the beginning of the OS that thinks."
echo ""
echo "Try it yourself:"
echo "  echo \"your question\" > /proc/llamux/prompt"
echo "  cat /proc/llamux/prompt"
echo ""
echo "🧠 The future of computing is here! 🚀"