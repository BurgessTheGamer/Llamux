/*
 * GGUF Parser for Llamux
 * 
 * Parses GGUF (GPT-Generated Unified Format) model files in kernel space.
 * This is a minimal implementation focused on TinyLlama support.
 *
 * GGUF Format: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md
 */

#ifndef _LLAMUX_GGUF_PARSER_H
#define _LLAMUX_GGUF_PARSER_H

#include <linux/types.h>

/* Forward declarations */
struct ggml_context;
struct ggml_tensor;

/* GGUF magic number */
#define GGUF_MAGIC 0x46554747  /* "GGUF" */
#define GGUF_VERSION 3

/* GGUF value types */
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

/* Tensor types for quantization */
enum ggml_type {
    GGML_TYPE_F32  = 0,
    GGML_TYPE_F16  = 1,
    GGML_TYPE_Q4_0 = 2,
    GGML_TYPE_Q4_1 = 3,
    GGML_TYPE_Q4_K = 12,
    GGML_TYPE_Q5_K = 13,
    GGML_TYPE_Q6_K = 14,
    GGML_TYPE_Q8_K = 15,
    GGML_TYPE_I32  = 16,
    GGML_TYPE_COUNT
};

/* GGUF header structure */
struct gguf_header {
    u32 magic;
    u32 version;
    u64 tensor_count;
    u64 metadata_kv_count;
} __packed;

/* GGUF string */
struct gguf_string {
    u64 length;
    char data[];  /* Variable length */
} __packed;

/* GGUF metadata key-value pair */
struct gguf_kv {
    struct gguf_string key;
    u32 value_type;
    /* Value follows based on type */
} __packed;

/* Tensor info */
struct gguf_tensor_info {
    char *name;
    u32 n_dims;
    u64 dims[4];      /* Max 4 dimensions */
    u32 type;         /* ggml_type */
    u64 offset;       /* Offset in file */
    size_t size;      /* Total size in bytes */
    void *data;       /* Pointer to tensor data in memory */
};

/* Parsed GGUF model */
struct gguf_model {
    struct gguf_header header;
    
    /* Model metadata */
    char *model_name;
    char *model_arch;
    u32 vocab_size;
    u32 context_length;
    u32 embedding_length;
    u32 n_layers;
    u32 n_heads;
    u32 n_heads_kv;
    u32 feed_forward_length;
    u32 rope_dimension_count;
    
    /* Tensors */
    struct gguf_tensor_info *tensors;
    u64 tensor_count;
    
    /* Memory layout */
    void *data;           /* Model data */
    size_t data_size;     /* Total data size */
    u64 data_offset;      /* Offset to tensor data in file */
};

/* Function prototypes */
int gguf_parse_header(const void *data, size_t size, struct gguf_header *header);
int gguf_parse_metadata(const void *data, size_t size, struct gguf_model *model);
int gguf_parse_tensor_info(const void *data, size_t size, struct gguf_model *model);
int gguf_load_tensor_data(const void *file_data, size_t file_size, struct gguf_model *model, void *tensor_memory, size_t memory_size);
int gguf_validate_model(struct gguf_model *model);
void gguf_free_model(struct gguf_model *model);
void gguf_print_model_info(struct gguf_model *model);
struct gguf_tensor_info *gguf_find_tensor(struct gguf_model *model, const char *name);
struct ggml_tensor *gguf_tensor_to_ggml(struct ggml_context *ctx, struct gguf_tensor_info *info);

/* Tensor operations */
size_t gguf_tensor_size(enum ggml_type type, int64_t n_elements);

#endif /* _LLAMUX_GGUF_PARSER_H */