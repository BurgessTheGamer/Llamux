#!/bin/bash
set -e

echo "Creating minimal test model..."

# Create a tiny test GGUF file (1MB instead of 638MB)
cd /root/Llamux
dd if=/dev/zero of=test-model.gguf bs=1M count=1

# Create test firmware directory
sudo mkdir -p /lib/firmware/llamux-test
sudo cp test-model.gguf /lib/firmware/llamux-test/

# Modify the module to use test model
cd /root/Llamux/llamux/kernel/llama_core

# Create a test version of main.c
cp main.c main.c.original
sed -i 's|/lib/firmware/llamux/tinyllama.gguf|/lib/firmware/llamux-test/test-model.gguf|g' main.c

# Also reduce memory in llama_model.c
sed -i 's/768 \* 1024 \* 1024/16 * 1024 * 1024/g' llama_model.c

# Rebuild
make clean && make

echo "Test module built with minimal requirements"