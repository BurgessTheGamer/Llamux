#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define GGUF_MAGIC 0x46554747

struct gguf_header {
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count;
    uint64_t metadata_kv_count;
} __attribute__((packed));

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <gguf_file>\n", argv[0]);
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
        close(fd);
        return 1;
    }

    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    struct gguf_header *header = (struct gguf_header *)data;
    printf("GGUF version: %u\n", header->version);
    printf("Tensor count: %lu\n", header->tensor_count);
    printf("Metadata count: %lu\n", header->metadata_kv_count);

    // Skip metadata parsing - go directly to tensor info
    uint8_t *ptr = (uint8_t *)data + sizeof(struct gguf_header);
    
    // Skip all metadata
    for (uint64_t i = 0; i < header->metadata_kv_count; i++) {
        // Skip key string
        uint64_t key_len = *(uint64_t *)ptr;
        ptr += sizeof(uint64_t) + key_len;
        
        // Skip value based on type
        uint32_t value_type = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        
        // Skip value data (simplified - just advance past common types)
        switch (value_type) {
            case 8: // STRING
                {
                    uint64_t str_len = *(uint64_t *)ptr;
                    ptr += sizeof(uint64_t) + str_len;
                }
                break;
            case 9: // ARRAY
                {
                    uint32_t arr_type = *(uint32_t *)ptr;
                    ptr += sizeof(uint32_t);
                    uint64_t arr_len = *(uint64_t *)ptr;
                    ptr += sizeof(uint64_t);
                    
                    // Skip array elements
                    for (uint64_t j = 0; j < arr_len; j++) {
                        if (arr_type == 8) { // STRING array
                            uint64_t elem_len = *(uint64_t *)ptr;
                            ptr += sizeof(uint64_t) + elem_len;
                        } else {
                            // Skip other types (simplified)
                            ptr += 8;
                        }
                    }
                }
                break;
            default:
                // Skip 4 or 8 bytes for other types
                ptr += (value_type <= 7) ? 4 : 8;
                break;
        }
    }
    
    // Now at tensor info
    printf("\nAll tensor names:\n");
    for (uint64_t i = 0; i < header->tensor_count && i < 400; i++) {
        // Read tensor name
        uint64_t name_len = *(uint64_t *)ptr;
        ptr += sizeof(uint64_t);
        
        char name[256];
        if (name_len < sizeof(name)) {
            memcpy(name, ptr, name_len);
            name[name_len] = '\0';
            printf("[%lu] %s\n", i, name);
        }
        ptr += name_len;
        
        // Skip tensor info
        uint32_t n_dims = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        ptr += n_dims * sizeof(uint64_t); // dimensions
        ptr += sizeof(uint32_t); // type
        ptr += sizeof(uint64_t); // offset
    }

    munmap(data, st.st_size);
    close(fd);
    return 0;
}