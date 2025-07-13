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
int gguf_validate_model(struct gguf_model *model);
void gguf_free_model(struct gguf_model *model);

/* Tensor operations */
size_t ggml_type_size(enum ggml_type type);
size_t ggml_tensor_size(const struct gguf_tensor_info *tensor);
const char *ggml_type_name(enum ggml_type type);

/* Utility functions */
void gguf_print_model_info(struct gguf_model *model);

#endif /* _LLAMUX_GGUF_PARSER_H */