#!/bin/bash
# Quick test to check actual tensor data requirements

echo "Checking TinyLlama tensor data requirements..."

# The GGUF file
MODEL="/lib/firmware/llamux/tinyllama.gguf"
SIZE=$(stat -c%s "$MODEL")
echo "Model file size: $((SIZE / 1024 / 1024)) MB"

# Check with strings for tensor names
echo ""
echo "First few tensor names in GGUF:"
strings "$MODEL" | grep -E "^(token_embd|output|blk\.[0-9]+\.)" | head -10

echo ""
echo "The issue is that the tensor loading code is calculating sizes incorrectly."
echo "The GGUF file stores tensor metadata with sizes, but these might be"
echo "getting misinterpreted, causing massive memory usage."