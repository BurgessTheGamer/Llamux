FROM debian:12

# Install kernel headers and build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    linux-headers-amd64 \
    kmod \
    wget \
    git \
    bc \
    libelf-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the Llamux source
COPY llamux /llamux

WORKDIR /llamux

# Build script
RUN echo '#!/bin/bash\n\
echo "🦙 Llamux Docker Test Environment"\n\
echo "================================="\n\
echo ""\n\
echo "Available commands:"\n\
echo "  make -C kernel/llama_core    - Build kernel module"\n\
echo "  make -C kernel/llama_core test - Test module (requires --privileged)"\n\
echo ""\n\
cd /llamux\n\
exec bash' > /entrypoint.sh && chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
