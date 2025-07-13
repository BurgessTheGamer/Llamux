#!/bin/bash
#
# Test script for Llamux kernel module
# This script loads the module, runs tests, and unloads it

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Find module path
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_PATH="$SCRIPT_DIR/../kernel/llama_core/llama_core.ko"

echo "🦙 Llamux Module Test Suite"
echo "=========================="
echo ""

# Function to run a test
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected="$3"
    
    echo -n "Testing $test_name... "
    
    if eval "$test_cmd"; then
        echo -e "${GREEN}PASSED${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAILED${NC}"
        echo "  Expected: $expected"
        ((TESTS_FAILED++))
    fi
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

# Check if module exists
if [ ! -f "$MODULE_PATH" ]; then
    echo -e "${RED}Error: Module not found at $MODULE_PATH${NC}"
    echo "Please build the module first with: make -C kernel/llama_core"
    exit 1
fi

# Remove module if already loaded
if lsmod | grep -q llama_core; then
    echo "Removing existing module..."
    rmmod llama_core
    sleep 1
fi

# Clear kernel log
dmesg -C

# Test 1: Module Loading
echo -e "\n${YELLOW}Test Group 1: Module Loading${NC}"
run_test "module insertion" "insmod $MODULE_PATH" "Module loads without errors"

# Test 2: Check proc interface
echo -e "\n${YELLOW}Test Group 2: Proc Interface${NC}"
run_test "/proc/llamux exists" "[ -d /proc/llamux ]" "/proc/llamux directory exists"
run_test "/proc/llamux/status exists" "[ -f /proc/llamux/status ]" "Status file exists"
run_test "status file readable" "cat /proc/llamux/status > /dev/null" "Status file is readable"

# Test 3: Check kernel messages
echo -e "\n${YELLOW}Test Group 3: Kernel Messages${NC}"
run_test "initialization message" "dmesg | grep -q 'Waking up the llama'" "Init message in dmesg"
run_test "thread started" "dmesg | grep -q 'Inference thread started'" "Thread start message"

# Test 4: Status content
echo -e "\n${YELLOW}Test Group 4: Status Content${NC}"
STATUS_CONTENT=$(cat /proc/llamux/status)
run_test "version info" "echo '$STATUS_CONTENT' | grep -q 'Version:'" "Version displayed"
run_test "initialized status" "echo '$STATUS_CONTENT' | grep -q 'Initialized: Yes'" "Module initialized"
run_test "thread running" "echo '$STATUS_CONTENT' | grep -q 'Inference Thread: Running'" "Thread is running"

# Test 5: Memory information
echo -e "\n${YELLOW}Test Group 5: Memory Status${NC}"
run_test "memory section" "echo '$STATUS_CONTENT' | grep -q 'Memory Status:'" "Memory section exists"
run_test "model info" "echo '$STATUS_CONTENT' | grep -q 'Model Information:'" "Model section exists"

# Show full status
echo -e "\n${YELLOW}Current Module Status:${NC}"
echo "----------------------"
cat /proc/llamux/status

# Test 6: Module unloading
echo -e "\n${YELLOW}Test Group 6: Module Unloading${NC}"
run_test "module removal" "rmmod llama_core" "Module unloads cleanly"
run_test "cleanup message" "dmesg | grep -q 'Putting the llama to sleep'" "Cleanup message"

# Summary
echo -e "\n${YELLOW}Test Summary:${NC}"
echo "============="
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed${NC}"
    exit 1
fi