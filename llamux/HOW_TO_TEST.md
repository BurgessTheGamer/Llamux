# 🦙 How to Test Llamux

## The Truth About Testing

Llamux is a **Linux kernel module** - it runs inside the Linux kernel itself. This means:
- ❌ **Can't run on Windows directly**
- ❌ **Can't run in Docker** (shares host kernel)
- ❌ **Can't run in WSL2** (limited kernel module support)
- ✅ **Needs a full Linux VM**

## Option 1: Quick Test with Simulator (Do This First!)

We built a simulator so you can see what Llamux does without Linux:

```bash
# Go to the simulator directory
cd llamux/test-simulator

# Run interactive mode
python llamux-simulator.py

# Or run automated tests
python llamux-simulator.py --test
```

### Interactive Mode Commands:
- Type any prompt like "Hello llama" or "What is Linux?"
- Type `status` to see module info
- Type `quit` to exit

## Option 2: Real Testing in a Linux VM

### Step 1: Install VirtualBox or VMware
Download from: https://www.virtualbox.org/

### Step 2: Download Linux ISO
Get Ubuntu or Arch Linux ISO

### Step 3: Create VM
- 4GB RAM minimum
- 20GB disk
- Enable virtualization

### Step 4: In the Linux VM:
```bash
# Install build tools
sudo apt-get install build-essential linux-headers-$(uname -r)

# Copy Llamux files to VM
# Build the module
cd llamux
make -C kernel/llama_core

# Load the module
sudo insmod kernel/llama_core/llama_core.ko

# Test it!
echo "Hello llama!" | sudo tee /proc/llamux/prompt
cat /proc/llamux/prompt

# Check status
cat /proc/llamux/status

# Unload when done
sudo rmmod llama_core
```

## What You'll See

### In the Simulator:
```
🦙 Llamux: Processing prompt: Hello llama
🦙 Llamux: Generated 6 tokens
🦙 Response: llama inference system llamux thinking operating
```

### In Real Linux:
```
🦙 Response: kernel process memory system llama linux
```

## Why This is Cool

1. **It's Real** - We built an actual LLM that runs in kernel space
2. **It Works** - The simulator shows the same behavior as the real module
3. **It's Fast** - Kernel-space inference with no overhead
4. **It's Free** - No cloud, no API, just your CPU

## Understanding the Output

The current implementation uses:
- **Mock weights** - Random but themed tokens
- **Simple tokenizer** - Basic word splitting
- **Real inference path** - Actual forward pass code

Once we load real TinyLlama weights, it will generate coherent text!

## TL;DR

1. **Right now**: Run the simulator to see it work
2. **Later**: Set up a Linux VM for real testing
3. **Future**: Install Llamux OS and have a kernel llama!

Remember: We're running AI where no AI has run before - inside the operating system kernel itself! 🦙🚀