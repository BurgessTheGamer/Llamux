#!/bin/bash
#
# Download TinyLlama model for Llamux
# This script downloads the quantized GGUF model file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODEL_DIR="$PROJECT_ROOT/models"

# Model details
MODEL_URL="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
MODEL_NAME="tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
MODEL_SIZE="637M"

echo "🦙 Llamux Model Downloader"
echo "========================="
echo ""

# Create models directory
mkdir -p "$MODEL_DIR"

# Check if model already exists
if [ -f "$MODEL_DIR/$MODEL_NAME" ]; then
    echo "✓ Model already downloaded: $MODEL_NAME"
    echo "  Location: $MODEL_DIR/$MODEL_NAME"
    exit 0
fi

echo "📥 Downloading TinyLlama-1.1B (Q4_K_M quantized)..."
echo "   Size: ~$MODEL_SIZE"
echo "   This may take a few minutes..."
echo ""

# Download with progress bar
if command -v wget &> /dev/null; then
    wget --show-progress -O "$MODEL_DIR/$MODEL_NAME" "$MODEL_URL"
elif command -v curl &> /dev/null; then
    curl -L --progress-bar -o "$MODEL_DIR/$MODEL_NAME" "$MODEL_URL"
else
    echo "❌ Error: Neither wget nor curl found. Please install one of them."
    exit 1
fi

# Verify download
if [ -f "$MODEL_DIR/$MODEL_NAME" ]; then
    ACTUAL_SIZE=$(du -h "$MODEL_DIR/$MODEL_NAME" | cut -f1)
    echo ""
    echo "✅ Model downloaded successfully!"
    echo "   File: $MODEL_NAME"
    echo "   Size: $ACTUAL_SIZE"
    echo "   Location: $MODEL_DIR/$MODEL_NAME"
else
    echo "❌ Error: Download failed!"
    exit 1
fi

echo ""
echo "🦙 Ready to build Llamux with TinyLlama!"