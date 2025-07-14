# Makefile for Llamux Core Kernel Module
# 
# This builds the llama_core.ko kernel module which provides
# the foundation for running TinyLlama in kernel space.

# Module name
obj-m += llama_core.o

# Source files (don't include llama_core.o in the objects list)
llama_core-objs := main.o gguf_parser.o memory_reserve_simple.o ggml_kernel.o tokenizer.o llama_model.o llama_proc.o quantize.o

# Kernel source directory (update this for your system)
KDIR ?= /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Compiler flags
ccflags-y := -Wall -Wno-unused-function
ccflags-y += -DLLAMUX_DEBUG
# Enable SIMD optimizations
ccflags-y += -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2
# Enable AVX2 for modern CPUs (comment out if not supported)
ccflags-y += -mavx -mavx2 -mfma

# Build targets
all:
	@echo "🦙 Building Llamux kernel module..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	@echo "🦙 Cleaning Llamux build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.order *.symvers

install:
	@echo "🦙 Installing Llamux kernel module..."
	sudo insmod llama_core.ko

remove:
	@echo "🦙 Removing Llamux kernel module..."
	sudo rmmod llama_core

test: all
	@echo "🦙 Testing Llamux kernel module..."
	sudo insmod llama_core.ko
	@sleep 1
	@echo "🦙 Checking /proc/llamux/status:"
	@cat /proc/llamux/status
	@sleep 1
	sudo rmmod llama_core
	@echo "🦙 Test complete!"

# Show module info
info:
	modinfo llama_core.ko

# Check kernel messages
log:
	@echo "🦙 Recent Llamux kernel messages:"
	@sudo dmesg | grep -i llamux | tail -20

.PHONY: all clean install remove test info log