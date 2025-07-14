/*
 * Test GGUF Parser - Userspace test program
 * 
 * This program tests parsing the GGUF file format to understand
 * what needs to be implemented in the kernel module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define GGUF_MAGIC 0x46554747  // "GGUF" in little-endian
#define GGUF_VERSION 3

// GGUF value types
enum gguf_type {
    GGUF_TYPE_UINT8   = 0,
    GGUF_TYPE_INT8    = 1,
    GGUF_TYPE_UINT16  = 2,
    GGUF_TYPE_INT16   = 3,
    GGUF_TYPE_UINT32  = 4,
    GGUF_TYPE_INT32   = 5,
    GGUF_TYPE_FLOAT32 = 6,
    GGUF_TYPE_BOOL    = 7,
    GGUF_TYPE_STRING  = 8,
    GGUF_TYPE_ARRAY   = 9,
    GGUF_TYPE_UINT64  = 10,
    GGUF_TYPE_INT64   = 11,
    GGUF_TYPE_FLOAT64 = 12,
};

// Tensor types
enum ggml_type {
    GGML_TYPE_F32  = 0,
    GGML_TYPE_F16  = 1,
    GGML_TYPE_Q4_0 = 2,
    GGML_TYPE_Q4_1 = 3,
    GGML_TYPE_Q4_K = 12,
    GGML_TYPE_Q5_K = 13,
    GGML_TYPE_Q6_K = 14,
    GGML_TYPE_Q8_K = 15,
};

struct gguf_header {
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count;
    uint64_t metadata_kv_count;
};

const char *get_ggml_type_name(enum ggml_type type) {
    switch (type) {
    case GGML_TYPE_F32:  return "F32";
    case GGML_TYPE_F16:  return "F16";
    case GGML_TYPE_Q4_0: return "Q4_0";
    case GGML_TYPE_Q4_1: return "Q4_1";
    case GGML_TYPE_Q4_K: return "Q4_K";
    case GGML_TYPE_Q5_K: return "Q5_K";
    case GGML_TYPE_Q6_K: return "Q6_K";
    case GGML_TYPE_Q8_K: return "Q8_K";
    default: return "UNKNOWN";
    }
}

// Read a string from the file
char *read_string(uint8_t **ptr) {
    uint64_t len = *(uint64_t *)*ptr;
    *ptr += sizeof(uint64_t);
    
    char *str = malloc(len + 1);
    memcpy(str, *ptr, len);
    str[len] = '\0';
    *ptr += len;
    
    return str;
}

// Skip value based on type
void skip_value(uint8_t **ptr, enum gguf_type type) {
    switch (type) {
    case GGUF_TYPE_UINT8:
    case GGUF_TYPE_INT8:
    case GGUF_TYPE_BOOL:
        *ptr += 1;
        break;
    case GGUF_TYPE_UINT16:
    case GGUF_TYPE_INT16:
        *ptr += 2;
        break;
    case GGUF_TYPE_UINT32:
    case GGUF_TYPE_INT32:
    case GGUF_TYPE_FLOAT32:
        *ptr += 4;
        break;
    case GGUF_TYPE_UINT64:
    case GGUF_TYPE_INT64:
    case GGUF_TYPE_FLOAT64:
        *ptr += 8;
        break;
    case GGUF_TYPE_STRING:
        {
            char *str = read_string(ptr);
            free(str);
        }
        break;
    case GGUF_TYPE_ARRAY:
        {
            uint32_t arr_type = *(uint32_t *)*ptr;
            *ptr += sizeof(uint32_t);
            uint64_t arr_len = *(uint64_t *)*ptr;
            *ptr += sizeof(uint64_t);
            for (uint64_t i = 0; i < arr_len; i++) {
                skip_value(ptr, arr_type);
            }
        }
        break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <gguf_file>\n", argv[0]);
        return 1;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return 1;
    }
    
    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    uint8_t *ptr = (uint8_t *)data;
    
    // Read header
    struct gguf_header header;
    memcpy(&header, ptr, sizeof(header));
    ptr += sizeof(header);
    
    printf("GGUF Header:\n");
    printf("  Magic: 0x%08x (should be 0x%08x)\n", header.magic, GGUF_MAGIC);
    printf("  Version: %u\n", header.version);
    printf("  Tensor count: %lu\n", header.tensor_count);
    printf("  Metadata count: %lu\n", header.metadata_kv_count);
    printf("\n");
    
    // Parse metadata
    printf("Key Model Parameters:\n");
    for (uint64_t i = 0; i < header.metadata_kv_count; i++) {
        char *key = read_string(&ptr);
        uint32_t value_type = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        
        if (strcmp(key, "general.architecture") == 0 ||
            strcmp(key, "general.name") == 0 ||
            strstr(key, "vocab_size") != NULL ||
            strstr(key, "tokenizer") != NULL) {
            if (value_type == GGUF_TYPE_STRING) {
                char *value = read_string(&ptr);
                printf("  %s = %s\n", key, value);
                free(value);
            } else {
                skip_value(&ptr, value_type);
            }
        } else if (strcmp(key, "llama.context_length") == 0 ||
                   strcmp(key, "llama.embedding_length") == 0 ||
                   strcmp(key, "llama.block_count") == 0 ||
                   strcmp(key, "llama.attention.head_count") == 0 ||
                   strcmp(key, "llama.feed_forward_length") == 0 ||
                   strcmp(key, "llama.vocab_size") == 0) {
            if (value_type == GGUF_TYPE_UINT32) {
                uint32_t value = *(uint32_t *)ptr;
                ptr += sizeof(uint32_t);
                printf("  %s = %u\n", key, value);
            } else {
                skip_value(&ptr, value_type);
            }
        } else {
            skip_value(&ptr, value_type);
        }
        
        free(key);
    }
    
    printf("\n");
    
    // Calculate alignment
    uint64_t metadata_end = ptr - (uint8_t *)data;
    uint64_t alignment = 32;  // Default GGUF alignment
    uint64_t tensor_data_offset = (metadata_end + header.tensor_count * 384 + alignment - 1) & ~(alignment - 1);
    
    // Parse tensor info
    printf("Tensor Information (first 10):\n");
    for (uint64_t i = 0; i < header.tensor_count && i < 10; i++) {
        char *name = read_string(&ptr);
        uint32_t n_dims = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        
        printf("  [%lu] %s: ", i, name);
        
        uint64_t total_elements = 1;
        for (uint32_t j = 0; j < n_dims; j++) {
            uint64_t dim = *(uint64_t *)ptr;
            ptr += sizeof(uint64_t);
            total_elements *= dim;
            printf("%lu", dim);
            if (j < n_dims - 1) printf(" x ");
        }
        
        uint32_t type = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        
        uint64_t offset = *(uint64_t *)ptr;
        ptr += sizeof(uint64_t);
        
        printf(" [%s] @ offset %lu", get_ggml_type_name(type), offset);
        
        // Show actual data offset
        uint64_t actual_offset = tensor_data_offset + offset;
        printf(" (file offset: %lu)\n", actual_offset);
        
        free(name);
    }
    
    if (header.tensor_count > 10) {
        printf("  ... and %lu more tensors\n", header.tensor_count - 10);
    }
    
    printf("\nTotal tensors: %lu\n", header.tensor_count);
    printf("Tensor data starts at offset: %lu\n", tensor_data_offset);
    
    munmap(data, st.st_size);
    close(fd);
    
    return 0;
}