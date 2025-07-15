# Direct File Loading for Large Models

The Linux firmware API has limitations on file size (typically a few hundred MB max). For CodeLlama 13B (7.4GB), we need a different approach.

## Options:

### 1. Direct File I/O (kernel_read_file)
- Use `kernel_read_file()` or `kernel_read_file_from_path()`
- Bypasses firmware API limitations
- Can handle files of any size

### 2. Memory-mapped approach
- Use `filp_open()` and `vfs_read()` 
- Read in chunks to avoid huge allocations
- More control over memory usage

### 3. Split model files
- Split CodeLlama into multiple <500MB chunks
- Load each chunk via firmware API
- Concatenate in memory

### 4. Custom loading mechanism
- Implement our own file loading that reads directly from disk
- Stream the model data as needed

## Recommended Solution:

For now, let's use approach #3 (split files) as it's the quickest to implement and test.

```bash
# Split the model into 500MB chunks
cd /lib/firmware/llamux/
split -b 500M codellama-13b.gguf codellama-13b.part.

# This creates:
# codellama-13b.part.aa (500MB)
# codellama-13b.part.ab (500MB)
# ... etc
```

Then modify the loader to load each part sequentially.