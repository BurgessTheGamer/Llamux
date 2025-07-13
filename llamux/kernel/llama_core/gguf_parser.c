/*
 * GGUF Parser Implementation for Llamux
 * 
 * Minimal GGUF parser for loading TinyLlama models in kernel space.
 * This implementation focuses on the subset needed for inference.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/bug.h>
#include "gguf_parser.h"

/* Get size in bytes for each tensor type */
size_t ggml_type_size(enum ggml_type type)
{
    switch (type) {
    case GGML_TYPE_F32:  return 4;
    case GGML_TYPE_F16:  return 2;
    case GGML_TYPE_Q4_0: return 18; /* block size for Q4_0 */
    case GGML_TYPE_Q4_1: return 20; /* block size for Q4_1 */
    case GGML_TYPE_Q4_K: return 144; /* block size for Q4_K */
    case GGML_TYPE_Q5_K: return 176; /* block size for Q5_K */
    case GGML_TYPE_Q6_K: return 210; /* block size for Q6_K */
    case GGML_TYPE_Q8_K: return 292; /* block size for Q8_K */
    default:
        pr_err("🦙 Llamux: Unknown tensor type %d\n", type);
        return 0;
    }
}

/* Get tensor type name */
const char *ggml_type_name(enum ggml_type type)
{
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

/* Calculate total tensor size */
size_t ggml_tensor_size(const struct gguf_tensor_info *tensor)
{
    size_t total_elements = 1;
    int i;
    
    for (i = 0; i < tensor->n_dims; i++) {
        total_elements *= tensor->dims[i];
    }
    
    /* For quantized types, we need to calculate based on blocks */
    if (tensor->type >= GGML_TYPE_Q4_0 && tensor->type <= GGML_TYPE_Q8_K) {
        size_t block_size = 32; /* Standard block size for quantization */
        size_t n_blocks = (total_elements + block_size - 1) / block_size;
        return n_blocks * ggml_type_size(tensor->type);
    }
    
    return total_elements * ggml_type_size(tensor->type);
}

/* Parse GGUF header */
int gguf_parse_header(const void *data, size_t size, struct gguf_header *header)
{
    const u8 *ptr = data;
    
    if (size < sizeof(struct gguf_header)) {
        pr_err("🦙 Llamux: GGUF file too small for header\n");
        return -EINVAL;
    }
    
    /* Read header fields */
    memcpy(&header->magic, ptr, sizeof(u32));
    ptr += sizeof(u32);
    
    memcpy(&header->version, ptr, sizeof(u32));
    ptr += sizeof(u32);
    
    memcpy(&header->tensor_count, ptr, sizeof(u64));
    ptr += sizeof(u64);
    
    memcpy(&header->metadata_kv_count, ptr, sizeof(u64));
    
    /* Validate magic number */
    if (header->magic != GGUF_MAGIC) {
        pr_err("🦙 Llamux: Invalid GGUF magic: 0x%x\n", header->magic);
        return -EINVAL;
    }
    
    /* Check version */
    if (header->version != GGUF_VERSION) {
        pr_err("🦙 Llamux: Unsupported GGUF version: %u\n", header->version);
        return -EINVAL;
    }
    
    pr_info("🦙 Llamux: GGUF header parsed - version %u, %llu tensors, %llu metadata\n",
            header->version, header->tensor_count, header->metadata_kv_count);
    
    return 0;
}

/* Read a GGUF string from buffer */
static const u8 *read_gguf_string(const u8 *ptr, char **str)
{
    u64 length;
    
    /* Read string length */
    memcpy(&length, ptr, sizeof(u64));
    ptr += sizeof(u64);
    
    /* Allocate and copy string */
    *str = kzalloc(length + 1, GFP_KERNEL);
    if (!*str) {
        return NULL;
    }
    
    memcpy(*str, ptr, length);
    ptr += length;
    
    return ptr;
}

/* Parse model metadata (simplified - only extracts key fields) */
int gguf_parse_metadata(const void *data, size_t size, struct gguf_model *model)
{
    const u8 *ptr = data;
    const u8 *end = ptr + size;
    u64 i;
    
    /* Skip header */
    ptr += sizeof(struct gguf_header);
    
    /* Parse each metadata key-value pair */
    for (i = 0; i < model->header.metadata_kv_count; i++) {
        char *key = NULL;
        u32 value_type;
        
        /* Read key */
        ptr = read_gguf_string(ptr, &key);
        if (!ptr || ptr >= end) {
            pr_err("🦙 Llamux: Failed to read metadata key\n");
            return -EINVAL;
        }
        
        /* Read value type */
        memcpy(&value_type, ptr, sizeof(u32));
        ptr += sizeof(u32);
        
        /* Parse specific keys we care about */
        if (strcmp(key, "general.name") == 0 && value_type == GGUF_TYPE_STRING) {
            ptr = read_gguf_string(ptr, &model->model_name);
        } else if (strcmp(key, "general.architecture") == 0 && value_type == GGUF_TYPE_STRING) {
            ptr = read_gguf_string(ptr, &model->model_arch);
        } else if (strcmp(key, "llama.context_length") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->context_length, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else if (strcmp(key, "llama.embedding_length") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->embedding_length, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else if (strcmp(key, "llama.block_count") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->n_layers, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else if (strcmp(key, "llama.attention.head_count") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->n_heads, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else {
            /* Skip unknown metadata */
            /* TODO: Implement full value parsing */
            pr_debug("🦙 Llamux: Skipping metadata key: %s\n", key);
        }
        
        kfree(key);
        
        if (ptr >= end) {
            pr_err("🦙 Llamux: Metadata parsing overrun\n");
            return -EINVAL;
        }
    }
    
    return ptr - (const u8 *)data;
}

/* Parse tensor information */
int gguf_parse_tensor_info(const void *data, size_t size, struct gguf_model *model)
{
    const u8 *ptr = data;
    const u8 *end = ptr + size;
    u64 i, j;
    
    /* Skip to tensor info section (after header and metadata) */
    /* This is simplified - real implementation needs proper offset calculation */
    
    /* Allocate tensor array */
    model->tensors = kzalloc(model->header.tensor_count * sizeof(struct gguf_tensor_info), 
                            GFP_KERNEL);
    if (!model->tensors) {
        pr_err("🦙 Llamux: Failed to allocate tensor array\n");
        return -ENOMEM;
    }
    
    /* Parse each tensor */
    for (i = 0; i < model->header.tensor_count; i++) {
        struct gguf_tensor_info *tensor = &model->tensors[i];
        
        /* Read tensor name */
        ptr = read_gguf_string(ptr, &tensor->name);
        if (!ptr || ptr >= end) {
            pr_err("🦙 Llamux: Failed to read tensor name\n");
            return -EINVAL;
        }
        
        /* Read number of dimensions */
        memcpy(&tensor->n_dims, ptr, sizeof(u32));
        ptr += sizeof(u32);
        
        /* Read dimensions */
        for (j = 0; j < tensor->n_dims; j++) {
            memcpy(&tensor->dims[j], ptr, sizeof(u64));
            ptr += sizeof(u64);
        }
        
        /* Read tensor type */
        memcpy(&tensor->type, ptr, sizeof(u32));
        ptr += sizeof(u32);
        
        /* Read offset */
        memcpy(&tensor->offset, ptr, sizeof(u64));
        ptr += sizeof(u64);
        
        /* Calculate size */
        tensor->size = ggml_tensor_size(tensor);
        
        pr_debug("🦙 Llamux: Tensor %llu: %s, type=%s, dims=%llu", 
                i, tensor->name, ggml_type_name(tensor->type), tensor->n_dims);
    }
    
    model->tensor_count = model->header.tensor_count;
    
    return 0;
}

/* Validate model compatibility */
int gguf_validate_model(struct gguf_model *model)
{
    /* Check if this is a TinyLlama model */
    if (!model->model_arch || strcmp(model->model_arch, "llama") != 0) {
        pr_err("🦙 Llamux: Not a Llama architecture model\n");
        return -EINVAL;
    }
    
    /* Validate model parameters */
    if (model->n_layers == 0 || model->n_heads == 0) {
        pr_err("🦙 Llamux: Invalid model parameters\n");
        return -EINVAL;
    }
    
    /* Check if model size is reasonable for kernel */
    if (model->data_size > 2ULL * 1024 * 1024 * 1024) {
        pr_err("🦙 Llamux: Model too large for kernel (>2GB)\n");
        return -EINVAL;
    }
    
    pr_info("🦙 Llamux: Model validated - %s architecture, %u layers, %u heads\n",
            model->model_arch, model->n_layers, model->n_heads);
    
    return 0;
}

/* Free model resources */
void gguf_free_model(struct gguf_model *model)
{
    u64 i;
    
    if (!model)
        return;
    
    /* Free strings */
    kfree(model->model_name);
    kfree(model->model_arch);
    
    /* Free tensor info */
    if (model->tensors) {
        for (i = 0; i < model->tensor_count; i++) {
            kfree(model->tensors[i].name);
        }
        kfree(model->tensors);
    }
    
    /* Note: model->data should be freed by caller */
}

/* Print model information */
void gguf_print_model_info(struct gguf_model *model)
{
    pr_info("🦙 Llamux Model Information:\n");
    pr_info("  Name: %s\n", model->model_name ?: "Unknown");
    pr_info("  Architecture: %s\n", model->model_arch ?: "Unknown");
    pr_info("  Context Length: %u\n", model->context_length);
    pr_info("  Embedding Length: %u\n", model->embedding_length);
    pr_info("  Layers: %u\n", model->n_layers);
    pr_info("  Heads: %u\n", model->n_heads);
    pr_info("  Tensors: %llu\n", model->tensor_count);
    pr_info("  Data Size: %zu bytes (%.2f MB)\n", 
            model->data_size, model->data_size / (1024.0 * 1024.0));
}