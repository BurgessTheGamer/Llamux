# Main Makefile for Llamux Project
# 
# This coordinates building all components of Llamux

.PHONY: all kernel userspace clean install test download-model help

# Default target
all: kernel userspace

help:
	@echo "🦙 Llamux Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  make all          - Build everything"
	@echo "  make kernel       - Build kernel modules only"
	@echo "  make userspace    - Build userspace tools only"
	@echo "  make download-model - Download TinyLlama model"
	@echo "  make clean        - Clean all build artifacts"
	@echo "  make test         - Run tests"
	@echo "  make install      - Install Llamux (requires root)"
	@echo "  make help         - Show this help"

# Download model
download-model:
	@bash scripts/download-model.sh

# Build kernel modules
kernel:
	@echo "🦙 Building kernel modules..."
	@$(MAKE) -C kernel/llama_core
	@# TODO: Add other modules as they're implemented

# Build userspace tools
userspace:
	@echo "🦙 Building userspace tools..."
	@# TODO: Add when Rust tools are implemented
	@echo "   (Userspace tools not yet implemented)"

# Clean everything
clean:
	@echo "🦙 Cleaning build artifacts..."
	@$(MAKE) -C kernel/llama_core clean
	@rm -rf build/

# Run tests
test: kernel
	@echo "🦙 Running tests..."
	@$(MAKE) -C kernel/llama_core test

# Install (requires root)
install: all
	@echo "🦙 Installing Llamux..."
	@echo "   (Installation not yet implemented)"
	@# TODO: Implement installation

# Build script wrapper
build:
	@bash scripts/build-all.sh