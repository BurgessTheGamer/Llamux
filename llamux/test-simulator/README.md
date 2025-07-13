# Llamux Test Simulator

Since you can't load Linux kernel modules on Windows, this simulator shows you what Llamux would do!

## How to Run

### Interactive Mode (like the real thing):
```bash
python llamux-simulator.py
```

Then type prompts like:
- `Hello llama`
- `What is Linux?`
- `status` (to see module info)
- `quit` (to exit)

### Automated Tests:
```bash
python llamux-simulator.py --test
```

## What This Simulates

This shows you what would happen if you:
1. Loaded the real kernel module: `insmod llama_core.ko`
2. Wrote to the proc interface: `echo "Hello" > /proc/llamux/prompt`
3. Read the response: `cat /proc/llamux/prompt`

## The Real Thing vs Simulator

| Feature | Real Llamux | Simulator |
|---------|------------|-----------|
| Runs in | Linux kernel | Python script |
| Memory | 2GB reserved at boot | Simulated |
| Inference | Real tensor ops | Random tokens |
| Speed | Kernel-fast | Sleep delays |
| Requires | Linux VM | Any OS |

## Try It Now!

This gives you a taste of what we built without needing Linux!