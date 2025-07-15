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
#include "ggml_kernel.h"

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
        size_t block_size;
        switch (tensor->type) {
        case GGML_TYPE_Q4_0:
        case GGML_TYPE_Q4_1:
            block_size = 32;
            break;
        case GGML_TYPE_Q4_K:
        case GGML_TYPE_Q5_K:
        case GGML_TYPE_Q6_K:
        case GGML_TYPE_Q8_K:
            block_size = 256; /* K-quants use 256-element super-blocks */
            break;
        default:
            block_size = 32;
        }
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
    if (header->version != GGUF_VERSION_V2 && header->version != GGUF_VERSION_V3) {
        pr_err("🦙 Llamux: Unsupported GGUF version: %u (supported: v2, v3)\n", header->version);
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
        pr_err("🦙 Llamux: Failed to allocate %llu bytes for string\n", length + 1);
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
    
    pr_info("🦙 Llamux: Parsing %llu metadata entries\n", model->header.metadata_kv_count);
    pr_info("🦙 Llamux: Starting at offset %ld, size %zu\n", ptr - (const u8 *)data, size);
    
    /* Parse each metadata key-value pair */
    for (i = 0; i < model->header.metadata_kv_count; i++) {
        char *key = NULL;
        u32 value_type;
        
        /* Read key */
        pr_debug("🦙 Llamux: Reading metadata key %llu at offset %ld\n", i, ptr - (const u8 *)data);
        ptr = read_gguf_string(ptr, &key);
        if (!ptr || ptr >= end) {
            pr_err("🦙 Llamux: Failed to read metadata key %llu at offset %ld\n", 
                   i, ptr ? (ptr - (const u8 *)data) : -1);
            return -EINVAL;
        }
        
        pr_info("🦙 Llamux: Metadata key %llu: '%s'\n", i, key);
        
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
        } else if (strcmp(key, "llama.attention.head_count_kv") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->n_heads_kv, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else if (strcmp(key, "llama.feed_forward_length") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->feed_forward_length, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else if (strcmp(key, "llama.rope.dimension_count") == 0 && value_type == GGUF_TYPE_UINT32) {
            memcpy(&model->rope_dimension_count, ptr, sizeof(u32));
            ptr += sizeof(u32);
        } else {
            /* Skip unknown metadata - need to properly parse value to advance pointer */
            switch (value_type) {
            case GGUF_TYPE_UINT8:
            case GGUF_TYPE_INT8:
            case GGUF_TYPE_BOOL:
                ptr += 1;
                break;
            case GGUF_TYPE_UINT16:
            case GGUF_TYPE_INT16:
                ptr += 2;
                break;
            case GGUF_TYPE_UINT32:
            case GGUF_TYPE_INT32:
            case GGUF_TYPE_FLOAT32:
                ptr += 4;
                break;
            case GGUF_TYPE_UINT64:
            case GGUF_TYPE_INT64:
            case GGUF_TYPE_FLOAT64:
                ptr += 8;
                break;
            case GGUF_TYPE_STRING:
                {
                    u64 str_len;
                    memcpy(&str_len, ptr, sizeof(u64));
                    ptr += sizeof(u64) + str_len;
                }
                break;
            case GGUF_TYPE_ARRAY:
                {
                    u32 arr_type;
                    u64 arr_len, j;
                    memcpy(&arr_type, ptr, sizeof(u32));
                    ptr += sizeof(u32);
                    memcpy(&arr_len, ptr, sizeof(u64));
                    ptr += sizeof(u64);
                    
                    /* Skip array elements based on type */
                    for (j = 0; j < arr_len; j++) {
                        switch (arr_type) {
                        case GGUF_TYPE_UINT32:
                        case GGUF_TYPE_INT32:
                        case GGUF_TYPE_FLOAT32:
                            ptr += 4;
                            break;
                        case GGUF_TYPE_FLOAT64:
                            ptr += 8;
                            break;
                        case GGUF_TYPE_STRING:
                            {
                                u64 str_len;
                                if (ptr + sizeof(u64) > end) {
                                    pr_err("🦙 Llamux: String array out of bounds\n");
                                    kfree(key);
                                    return -EINVAL;
                                }
                                memcpy(&str_len, ptr, sizeof(u64));
                                ptr += sizeof(u64);
                                
                                if (ptr + str_len > end) {
                                    pr_err("🦙 Llamux: String in array out of bounds (len=%llu)\n", str_len);
                                    kfree(key);
                                    return -EINVAL;
                                }
                                ptr += str_len;
                            }
                            break;
                        default:
                            pr_warn("🦙 Llamux: Unsupported array type %u\n", arr_type);
                            return -EINVAL;
                        }
                    }
                }
                break;
            default:
                pr_warn("🦙 Llamux: Unknown value type %u for key %s\n", value_type, key);
                break;
            }
        }
        
        kfree(key);
        
        if (ptr >= end) {
            pr_err("🦙 Llamux: Metadata parsing overrun at key %llu\n", i);
            return -EINVAL;
        }
    }
    
    pr_info("🦙 Llamux: Successfully parsed %llu metadata entries\n", 
            model->header.metadata_kv_count);
    return ptr - (const u8 *)data;
}

/* Parse tensor information */
int gguf_parse_tensor_info(const void *data, size_t size, struct gguf_model *model)
{
    const u8 *ptr = data;
    const u8 *end = ptr + size;
    u64 i, j;
    int metadata_size;
    
    /* Skip header */
    ptr += sizeof(struct gguf_header);
    
    /* Parse metadata again to get correct offset */
    metadata_size = gguf_parse_metadata(data, size, model);
    if (metadata_size < 0) {
        return metadata_size;
    }
    
    /* Move to tensor info section */
    ptr = data + metadata_size;
    
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
        
        pr_debug("🦙 Llamux: Tensor %llu: %s, type=%s, dims=%u", 
                i, tensor->name, ggml_type_name(tensor->type), tensor->n_dims);
    }
    
    model->tensor_count = model->header.tensor_count;
    
    /* Calculate data offset - align to 32 bytes */
    u64 tensor_info_end = ptr - (const u8 *)data;
    model->data_offset = (tensor_info_end + 31) & ~31ULL;
    
    pr_info("🦙 Llamux: Parsed %llu tensors, data starts at offset %llu\n",
            model->tensor_count, model->data_offset);
    
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

/* Load tensor data from GGUF file */
int gguf_load_tensor_data(const void *file_data, size_t file_size, struct gguf_model *model, void *tensor_memory, size_t memory_size)
{
    const u8 *file_ptr = (const u8 *)file_data;
    u8 *mem_ptr = (u8 *)tensor_memory;
    size_t total_size = 0;
    u64 i;
    
    if (!file_data || !model || !tensor_memory || !model->tensors) {
        pr_err("🦙 Llamux: Invalid parameters for tensor loading\n");
        return -EINVAL;
    }
    
    pr_info("🦙 Llamux: Loading tensor data from offset %llu\n", model->data_offset);
    pr_info("🦙 Llamux: Memory available: %zu MB, need ~%zu MB\n", 
            memory_size / (1024*1024), 
            (file_size - model->data_offset) / (1024*1024));
    
    /* Copy tensor data */
    for (i = 0; i < model->tensor_count; i++) {
        struct gguf_tensor_info *tensor = &model->tensors[i];
        
        if (tensor->offset + tensor->size > file_size - model->data_offset) {
            pr_err("🦙 Llamux: Tensor %s exceeds file bounds\n", tensor->name);
            return -EINVAL;
        }
        
        if (total_size + tensor->size > memory_size) {
            pr_err("🦙 Llamux: Not enough memory for tensor %s (need %zu MB, have %zu MB used, %zu MB total)\n", 
                   tensor->name, 
                   (total_size + tensor->size) / (1024*1024),
                   total_size / (1024*1024),
                   memory_size / (1024*1024));
            return -ENOMEM;
        }
        
        /* Copy tensor data */
        memcpy(mem_ptr + total_size, 
               file_ptr + model->data_offset + tensor->offset, 
               tensor->size);
        
        /* Update tensor data pointer */
        tensor->data = mem_ptr + total_size;
        total_size += tensor->size;
        
        pr_debug("🦙 Llamux: Loaded tensor %s (%zu bytes)\n", 
                 tensor->name, tensor->size);
    }
    
    pr_info("🦙 Llamux: Loaded %zu bytes of tensor data\n", total_size);
    return 0;
}

/* Print model info */
void gguf_print_model_info(struct gguf_model *model) {
    if (!model) return;
    
    pr_info("🦙 GGUF Model Information:\n");
    pr_info("  Name: %s\n", model->model_name ?: "Unknown");
    pr_info("  Architecture: %s\n", model->model_arch ?: "Unknown");
    pr_info("  Vocabulary: %u tokens\n", model->vocab_size);
    pr_info("  Context: %u tokens\n", model->context_length);
    pr_info("  Embedding: %u\n", model->embedding_length);
    pr_info("  Layers: %u\n", model->n_layers);
    pr_info("  Heads: %u\n", model->n_heads);
    pr_info("  Tensors: %llu\n", model->tensor_count);
    pr_info("  Data size: %zu MB\n", model->data_size / (1024 * 1024));
}

/* Find tensor by name */
struct gguf_tensor_info *gguf_find_tensor(struct gguf_model *model, const char *name) {
    int i;
    
    if (!model || !name) {
        return NULL;
    }
    
    /* Debug: print first few tensor names on first lookup */
    static int debug_printed = 0;
    if (!debug_printed && strcmp(name, "token_embd.weight") == 0) {
        pr_info("🦙 GGUF: Looking for tensor '%s' among %llu tensors:\n", name, model->tensor_count);
        for (i = 0; i < min(5, model->tensor_count); i++) {
            pr_info("🦙 GGUF:   Tensor %d: '%s'\n", i, model->tensors[i].name);
        }
        debug_printed = 1;
    }
    
    for (i = 0; i < model->tensor_count; i++) {
        if (strcmp(model->tensors[i].name, name) == 0) {
            return &model->tensors[i];
        }
    }
    
    return NULL;
}

/* Convert GGUF tensor to GGML tensor */
struct ggml_tensor *gguf_tensor_to_ggml(struct ggml_context *ctx, 
                                        struct gguf_tensor_info *info) {
    struct ggml_tensor *tensor;
    
    if (!ctx || !info || ctx->n_objects >= GGML_MAX_NODES) {
        return NULL;
    }
    
    /* Allocate tensor */
    tensor = kzalloc(sizeof(struct ggml_tensor), GFP_KERNEL);
    if (!tensor) {
        pr_err("🦙 GGUF: Failed to allocate tensor\n");
        return NULL;
    }
    
    /* Set tensor properties */
    tensor->type = info->type;
    tensor->n_dims = info->n_dims;
    for (int i = 0; i < GGML_MAX_DIMS; i++) {
        tensor->ne[i] = i < info->n_dims ? info->dims[i] : 1;
        tensor->nb[i] = i == 0 ? ggml_type_size(info->type) : 
                        tensor->nb[i-1] * tensor->ne[i-1];
    }
    
    /* Point to existing data */
    tensor->data = info->data;
    tensor->op = GGML_OP_NONE;
    tensor->src0 = NULL;
    tensor->src1 = NULL;
    
    /* Add to context */
    ctx->objects[ctx->n_objects++] = tensor;
    
    pr_info("🦙 GGUF: Created view tensor type=%d, dims=[%lld,%lld], data=%p\n",
            info->type, tensor->ne[0], tensor->ne[1], info->data);
    
    return tensor;
}

/* Calculate tensor size */
size_t gguf_tensor_size(enum ggml_type type, int64_t n_elements) {
    switch (type) {
        case GGML_TYPE_F32:
            return n_elements * sizeof(float);
        case GGML_TYPE_F16:
            return n_elements * sizeof(uint16_t);
        case GGML_TYPE_Q4_0:
            return (n_elements / 32) * 18;
        case GGML_TYPE_Q4_K:
            return (n_elements / 256) * 144;
        case GGML_TYPE_Q5_K:
            return (n_elements / 256) * 176;
        case GGML_TYPE_Q6_K:
            return (n_elements / 256) * 210;
        case GGML_TYPE_Q8_K:
            return (n_elements / 256) * 292;
        default:
            pr_warn("🦙 Llamux: Unknown tensor type %d\n", type);
            return 0;
    }
}