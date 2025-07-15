#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define GGUF_MAGIC 0x46554747

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

struct gguf_header {
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count;
    uint64_t metadata_kv_count;
};

char *read_string(uint8_t **ptr) {
    uint64_t len = *(uint64_t *)*ptr;
    *ptr += sizeof(uint64_t);
    
    char *str = malloc(len + 1);
    memcpy(str, *ptr, len);
    str[len] = '\0';
    *ptr += len;
    
    return str;
}

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
    
    struct gguf_header header;
    memcpy(&header, ptr, sizeof(header));
    ptr += sizeof(header);
    
    printf("GGUF Metadata Keys (%lu total):\n", header.metadata_kv_count);
    printf("Looking for tokenizer/vocabulary metadata...\n\n");
    
    for (uint64_t i = 0; i < header.metadata_kv_count; i++) {
        char *key = read_string(&ptr);
        uint32_t value_type = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        
        // Show all keys that contain "token" or "vocab"
        if (strstr(key, "token") != NULL || strstr(key, "vocab") != NULL) {
            printf("Key: %s (type=%u)\n", key, value_type);
            
            if (value_type == GGUF_TYPE_STRING) {
                char *value = read_string(&ptr);
                printf("  Value: %s\n", value);
                free(value);
            } else if (value_type == GGUF_TYPE_UINT32) {
                uint32_t value = *(uint32_t *)ptr;
                ptr += sizeof(uint32_t);
                printf("  Value: %u\n", value);
            } else if (value_type == GGUF_TYPE_ARRAY) {
                uint32_t arr_type = *(uint32_t *)ptr;
                ptr += sizeof(uint32_t);
                uint64_t arr_len = *(uint64_t *)ptr;
                ptr += sizeof(uint64_t);
                printf("  Array type=%u, length=%lu\n", arr_type, arr_len);
                if (arr_len > 5) {
                    printf("  (showing first 5 elements)\n");
                }
                for (uint64_t j = 0; j < arr_len && j < 5; j++) {
                    if (arr_type == GGUF_TYPE_STRING) {
                        char *elem = read_string(&ptr);
                        printf("    [%lu]: %s\n", j, elem);
                        free(elem);
                    } else {
                        skip_value(&ptr, arr_type);
                    }
                }
                if (arr_len > 5) {
                    for (uint64_t j = 5; j < arr_len; j++) {
                        skip_value(&ptr, arr_type);
                    }
                }
            } else {
                skip_value(&ptr, value_type);
            }
            printf("\n");
        } else {
            skip_value(&ptr, value_type);
        }
        
        free(key);
    }
    
    munmap(data, st.st_size);
    close(fd);
    
    return 0;
}
