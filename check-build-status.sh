#!/bin/bash

# Quick build status check

echo "🦙 Llamux Build Status"
echo "===================="
echo ""

# Check for .deb files
DEB_COUNT=$(find /root/Llamux/kernel-build -name "*.deb" 2>/dev/null | wc -l)

if [ $DEB_COUNT -gt 0 ]; then
    echo "✅ BUILD COMPLETE!"
    echo ""
    echo "Found packages:"
    find /root/Llamux/kernel-build -name "*.deb" -exec basename {} \; | sort
    echo ""
    echo "Ready to install or test in QEMU!"
else
    # Check if build is running
    BUILD_PROCS=$(ps aux | grep -E "make.*-j|cc1|as " | grep -v grep | wc -l)
    if [ $BUILD_PROCS -gt 0 ]; then
        echo "⚙️  Build in progress..."
        echo "   Active processes: $BUILD_PROCS"
        echo ""
        echo "Run ./monitor-build.sh to watch progress"
    else
        echo "❓ Build status unknown"
        echo "   No .deb files found"
        echo "   No active build processes"
    fi
fi