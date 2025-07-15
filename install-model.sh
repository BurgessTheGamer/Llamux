#!/bin/bash

# Install TinyLlama model to firmware directory for kernel loading

set -e

echo "🦙 Llamux Model Installer"
echo "========================"

MODEL_FILE="llamux/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
FIRMWARE_DIR="/lib/firmware/llamux"
TARGET_NAME="tinyllama.gguf"

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "❌ Please run as root (use sudo)"
    exit 1
fi

# Check if model exists
if [ ! -f "$MODEL_FILE" ]; then
    echo "❌ Model file not found: $MODEL_FILE"
    echo "   Run ./scripts/download-model.sh first"
    exit 1
fi

# Create firmware directory
echo "📁 Creating firmware directory..."
mkdir -p "$FIRMWARE_DIR"

# Copy model
echo "📦 Installing model to kernel firmware..."
cp -v "$MODEL_FILE" "$FIRMWARE_DIR/$TARGET_NAME"

# Verify
if [ -f "$FIRMWARE_DIR/$TARGET_NAME" ]; then
    SIZE=$(du -h "$FIRMWARE_DIR/$TARGET_NAME" | cut -f1)
    echo "✅ Model installed successfully!"
    echo "   Location: $FIRMWARE_DIR/$TARGET_NAME"
    echo "   Size: $SIZE"
else
    echo "❌ Installation failed!"
    exit 1
fi

echo ""
echo "🦙 Ready to load real model in kernel!"