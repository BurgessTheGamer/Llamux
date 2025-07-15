#!/bin/bash
echo "🔨 Building Llamux kernel modules..."
cd "$(dirname "$0")"
make -C llamux/kernel/llama_core clean
make -C llamux/kernel/llama_core
echo "✅ Build complete!"
