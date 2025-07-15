#!/bin/bash
echo "🚀 Starting Llamux in User Mode Linux..."
echo "   - Instant start (no VM boot)"
echo "   - 2GB vmalloc configured"
echo "   - Isolated from host"
echo ""

# Run with User Mode Linux
linux.uml \
    mem=2G \
    rootfstype=hostfs \
    rootflags=/ \
    rw \
    eth0=tuntap,,,192.168.0.1 \
    init=/init \
    vmalloc=2048M \
    quiet
