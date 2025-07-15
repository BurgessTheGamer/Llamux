#!/bin/bash
echo "🚀 Starting Llamux with systemd-nspawn..."
echo "   - Super fast container"
echo "   - Shares kernel but isolated"
echo ""

# Install systemd-container if needed
if ! command -v systemd-nspawn &> /dev/null; then
    apt-get install -y systemd-container
fi

# Run container
systemd-nspawn \
    --directory=rootfs \
    --capability=CAP_SYS_MODULE,CAP_SYS_ADMIN \
    --bind=/lib/modules \
    --setenv=VMALLOC=2048M \
    /bin/sh -c "
        echo '🦙 Llamux nspawn container'
        echo 'Note: This shares the host kernel'
        echo 'For full isolation, use run-container.sh'
        echo ''
        cd /root/llamux/llama_core
        ls -la
        /bin/sh
    "
