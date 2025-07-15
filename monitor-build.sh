#!/bin/bash

# Monitor Llamux kernel build progress

echo "🦙 Llamux Kernel Build Monitor"
echo "=============================="
echo ""

BUILD_DIR="/root/Llamux/kernel-build/linux-source-6.1"
CHECK_INTERVAL=30

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

while true; do
    clear
    echo "🦙 Llamux Kernel Build Monitor"
    echo "=============================="
    echo "Time: $(date)"
    echo ""
    
    # Check if build is running
    BUILD_PROCS=$(ps aux | grep -E "make.*-j|cc1|as " | grep -v grep | wc -l)
    if [ $BUILD_PROCS -gt 0 ]; then
        echo -e "${YELLOW}⚙️  Build Status: IN PROGRESS${NC}"
        echo "   Active processes: $BUILD_PROCS"
    else
        echo -e "${GREEN}✓ Build Status: IDLE/COMPLETE${NC}"
    fi
    echo ""
    
    # Check for .deb files
    echo "📦 Checking for built packages..."
    DEB_COUNT=$(find $BUILD_DIR/.. -name "*.deb" 2>/dev/null | wc -l)
    if [ $DEB_COUNT -gt 0 ]; then
        echo -e "${GREEN}✅ Found $DEB_COUNT .deb packages:${NC}"
        find $BUILD_DIR/.. -name "*.deb" -exec basename {} \; | sort
        echo ""
        echo -e "${GREEN}🎉 BUILD COMPLETE!${NC}"
        echo ""
        echo "Next steps:"
        echo "1. Install: cd $BUILD_DIR/.. && sudo dpkg -i linux-image-*-llamux*.deb"
        echo "2. Or test in QEMU first (see test-in-qemu.sh)"
        break
    else
        echo "   No packages found yet..."
    fi
    echo ""
    
    # Show recent build output
    echo "📋 Recent build activity:"
    if [ -f "$BUILD_DIR/debian/build/build.log" ]; then
        tail -5 "$BUILD_DIR/debian/build/build.log" 2>/dev/null | sed 's/^/   /'
    else
        # Try to find make output
        ps aux | grep -E "make|cc1" | grep -v grep | head -3 | awk '{print "   " $11 " " $12 " " $13}'
    fi
    echo ""
    
    # System resources
    echo "💻 System resources:"
    echo "   CPU: $(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1)% used"
    echo "   RAM: $(free -h | awk '/^Mem:/{print $3 " / " $2}')"
    echo "   Load: $(uptime | awk -F'load average:' '{print $2}')"
    echo ""
    
    echo "Refreshing in $CHECK_INTERVAL seconds... (Ctrl+C to stop)"
    sleep $CHECK_INTERVAL
done