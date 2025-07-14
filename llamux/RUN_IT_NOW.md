# 🚀 RUN LLAMUX RIGHT NOW!

## Quick Test (2 minutes)

### Terminal 1 - Load the module:
```bash
cd /root/Idea/llamux
sudo bash tests/test_module.sh
```

### Terminal 2 - Talk to the AI:
```bash
# Send a prompt
echo "Hello AI, what are you?" | sudo tee /proc/llamux/prompt

# Read the response
sudo cat /proc/llamux/prompt
```

## Full Experience (5 minutes)

### 1. Download the model (if not already done):
```bash
make download-model
```

### 2. Build everything:
```bash
make clean && make all
```

### 3. Load the kernel module:
```bash
sudo insmod kernel/llama_core/llama_core.ko
```

### 4. Check it loaded:
```bash
dmesg | tail -20  # You'll see "🦙 Llamux: The AI has awakened!"
```

### 5. Use the Llama Shell:
```bash
cd userspace/lsh
make
sudo ./lsh
```

Then type natural language commands like:
- "show me the files here"
- "what time is it"
- "create a file called hello.txt"

## What You'll See

1. The kernel module loads with a personality message
2. The AI responds through /proc/llamux/prompt
3. The Llama Shell translates your words to commands
4. Your Linux system is now THINKING! 🧠

## If Something Goes Wrong

```bash
# Remove the module
sudo rmmod llama_core

# Check kernel logs
dmesg | grep -i llamux
```

IT'S ALIVE! 🦙⚡