/*
 * TinyLlama Model Implementation for Llamux
 * 
 * Core inference engine for running TinyLlama in kernel space.
 * This is a simplified implementation focusing on basic functionality.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <asm/fpu/api.h>
#include "llama_model.h"
#include "ggml_kernel.h"
#include "gguf_parser.h"
#include "llamux_stats.h"

/* Create model structure from GGUF data */
struct llama_model *llama_model_create_from_gguf(struct ggml_context *ctx, struct gguf_model *gguf) {
    struct llama_model *model;
    int i;
    
    if (!ctx || !gguf) {
        pr_err("🦙 Llama: Invalid parameters for GGUF model creation\n");
        return NULL;
    }
    
    model = kzalloc(sizeof(struct llama_model), GFP_KERNEL);
    if (!model) {
        pr_err("🦙 Llama: Failed to allocate model\n");
        return NULL;
    }
    
    /* Set hyperparameters from GGUF */
    model->hparams.n_vocab = gguf->vocab_size ?: 32000; /* Use GGUF vocab_size or default */
    model->hparams.n_ctx = gguf->context_length;
    model->hparams.n_embd = gguf->embedding_length;
    model->hparams.n_head = gguf->n_heads;
    model->hparams.n_layer = gguf->n_layers;
    model->hparams.n_ff = gguf->feed_forward_length;
    model->hparams.n_rot = gguf->rope_dimension_count;
    model->hparams.f_norm_eps = 1e-5f;
    model->hparams.rope_theta = LLAMA_ROPE_THETA;
    
    model->ctx = ctx;
    
    /* Allocate layers */
    model->layers = kzalloc(model->hparams.n_layer * sizeof(struct llama_layer), 
                           GFP_KERNEL);
    if (!model->layers) {
        pr_err("🦙 Llama: Failed to allocate layers\n");
        kfree(model);
        return NULL;
    }
    
    /* Map GGUF tensors to model weights */
    pr_info("🦙 Llama: Mapping GGUF tensors to model weights...\n");
    
    /* Token embeddings */
    struct gguf_tensor_info *tok_embd_gguf = gguf_find_tensor(gguf, "token_embd.weight");
    if (tok_embd_gguf) {
        model->tok_embeddings = gguf_tensor_to_ggml(ctx, tok_embd_gguf);
        if (!model->tok_embeddings) {
            pr_err("🦙 Llama: Failed to create token embeddings tensor!\n");
            kfree(model->layers);
            kfree(model);
            return NULL;
        }
        pr_info("🦙 Llama: Token embeddings created: %p, dims: %lld x %lld\n", 
                model->tok_embeddings, tok_embd_gguf->dims[0], tok_embd_gguf->dims[1]);
    } else {
        pr_err("🦙 Llama: Token embeddings not found - required for real model!\n");
        kfree(model->layers);
        kfree(model);
        return NULL;
    }
    
    /* Output norm */
    struct gguf_tensor_info *norm_gguf = gguf_find_tensor(gguf, "output_norm.weight");
    if (norm_gguf) {
        model->norm = gguf_tensor_to_ggml(ctx, norm_gguf);
    } else {
        pr_warn("🦙 Llama: Output norm not found\n");
    }
    
    /* Output projection */
    struct gguf_tensor_info *output_gguf = gguf_find_tensor(gguf, "output.weight");
    if (output_gguf) {
        model->output = gguf_tensor_to_ggml(ctx, output_gguf);
    } else {
        pr_warn("🦙 Llama: Output projection not found\n");
    }
    
    /* Initialize tokenizer with GGUF vocabulary if available */
    if (gguf->vocab_tokens && gguf->vocab_size > 0) {
        pr_info("🦙 Llama: Initializing tokenizer with GGUF vocabulary (%u tokens)\n", gguf->vocab_size);
        if (llama_tokenizer_init_from_gguf(&model->tokenizer, 
                                          gguf->vocab_tokens, gguf->vocab_size,
                                          gguf->bos_token_id, gguf->eos_token_id,
                                          gguf->unk_token_id, gguf->pad_token_id) != 0) {
            pr_warn("🦙 Llama: Failed to init tokenizer from GGUF, using simple tokenizer\n");
            llama_tokenizer_init(&model->tokenizer);
        }
    } else {
        pr_info("🦙 Llama: No vocabulary in GGUF, using simple tokenizer\n");
        llama_tokenizer_init(&model->tokenizer);
    }
    
    /* Map layer weights */
    for (i = 0; i < model->hparams.n_layer; i++) {
        struct llama_layer *layer = &model->layers[i];
        char tensor_name[64];
        struct gguf_tensor_info *gguf_tensor;
        
        /* Attention weights */
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.attn_q.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->wq = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->wq) strncpy(layer->wq->name, tensor_name, GGML_MAX_NAME-1);
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.attn_k.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->wk = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->wk) strncpy(layer->wk->name, tensor_name, GGML_MAX_NAME-1);
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.attn_v.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->wv = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->wv) strncpy(layer->wv->name, tensor_name, GGML_MAX_NAME-1);
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.attn_output.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->wo = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->wo) strncpy(layer->wo->name, tensor_name, GGML_MAX_NAME-1);
        
        /* FFN weights - we'll transpose w1 and w3 after loading */
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.ffn_gate.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->w1 = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->w1) strncpy(layer->w1->name, tensor_name, GGML_MAX_NAME-1);
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.ffn_down.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->w2 = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->w2) strncpy(layer->w2->name, tensor_name, GGML_MAX_NAME-1);
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.ffn_up.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->w3 = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        if (layer->w3) strncpy(layer->w3->name, tensor_name, GGML_MAX_NAME-1);
        
        /* Don't transpose during loading - we'll handle it differently */
        
        /* Normalization */
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.attn_norm.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->attention_norm = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        
        snprintf(tensor_name, sizeof(tensor_name), "blk.%d.ffn_norm.weight", i);
        gguf_tensor = gguf_find_tensor(gguf, tensor_name);
        layer->ffn_norm = gguf_tensor ? gguf_tensor_to_ggml(ctx, gguf_tensor) : NULL;
        
        if (!layer->wq || !layer->wk || !layer->wv || !layer->wo) {
            pr_warn("🦙 Llama: Missing attention weights for layer %d, creating placeholders\n", i);
            /* Create placeholder attention weights */
            int64_t ne_qkv[4] = {model->hparams.n_embd, model->hparams.n_embd, 1, 1};
            int64_t ne_o[4] = {model->hparams.n_embd, model->hparams.n_embd, 1, 1};
            
            if (!layer->wq) layer->wq = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_qkv);
            if (!layer->wk) layer->wk = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_qkv);
            if (!layer->wv) layer->wv = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_qkv);
            if (!layer->wo) layer->wo = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_o);
        }
        if (!layer->w1 || !layer->w2 || !layer->w3) {
            pr_warn("🦙 Llama: Missing FFN weights for layer %d, creating placeholders\n", i);
            /* Create placeholder FFN weights */
            int64_t ne_w1[4] = {model->hparams.n_embd, model->hparams.n_ff, 1, 1};
            int64_t ne_w2[4] = {model->hparams.n_ff, model->hparams.n_embd, 1, 1};
            int64_t ne_w3[4] = {model->hparams.n_embd, model->hparams.n_ff, 1, 1};
            
            if (!layer->w1) layer->w1 = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_w1);
            if (!layer->w2) layer->w2 = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_w2);
            if (!layer->w3) layer->w3 = ggml_new_tensor(ctx, GGML_TYPE_F32, 2, ne_w3);
        }
        if (!layer->attention_norm || !layer->ffn_norm) {
            pr_warn("🦙 Llama: Missing norm weights for layer %d, creating placeholders\n", i);
            int64_t ne_norm[4] = {model->hparams.n_embd, 1, 1, 1};
            
            if (!layer->attention_norm) layer->attention_norm = ggml_new_tensor(ctx, GGML_TYPE_F32, 1, ne_norm);
            if (!layer->ffn_norm) layer->ffn_norm = ggml_new_tensor(ctx, GGML_TYPE_F32, 1, ne_norm);
        }
    }
    
    /* Initialize tokenizer */
    if (llama_tokenizer_init(&model->tokenizer) != 0) {
        pr_err("🦙 Llama: Failed to initialize tokenizer\n");
        kfree(model->layers);
        kfree(model);
        return NULL;
    }
    
    pr_info("🦙 Llama: Real model created from GGUF - %d layers, %d embd, %d heads\n",
            model->hparams.n_layer, model->hparams.n_embd, model->hparams.n_head);
    
    return model;
}

/* Create model structure */
struct llama_model *llama_model_create(struct ggml_context *ctx) {
    struct llama_model *model;
    
    model = kzalloc(sizeof(struct llama_model), GFP_KERNEL);
    if (!model) {
        pr_err("🦙 Llama: Failed to allocate model\n");
        return NULL;
    }
    
    /* Set default hyperparameters for TinyLlama-1.1B */
    model->hparams.n_vocab = LLAMA_N_VOCAB;
    model->hparams.n_ctx = LLAMA_N_CTX;
    model->hparams.n_embd = LLAMA_N_EMBD;
    model->hparams.n_head = LLAMA_N_HEAD;
    model->hparams.n_layer = LLAMA_N_LAYER;
    model->hparams.n_ff = LLAMA_N_FF;
    model->hparams.n_rot = LLAMA_ROPE_DIM;
    model->hparams.f_norm_eps = 1e-5f;
    model->hparams.rope_theta = LLAMA_ROPE_THETA;
    
    model->ctx = ctx;
    
    /* Allocate layers */
    model->layers = kzalloc(model->hparams.n_layer * sizeof(struct llama_layer), 
                           GFP_KERNEL);
    if (!model->layers) {
        pr_err("🦙 Llama: Failed to allocate layers\n");
        kfree(model);
        return NULL;
    }
    
    /* Initialize tokenizer */
    if (llama_tokenizer_init(&model->tokenizer) != 0) {
        pr_err("🦙 Llama: Failed to initialize tokenizer\n");
        kfree(model->layers);
        kfree(model);
        return NULL;
    }
    
    /* Initialize weight cache - 15GB for dequantized weights */
    model->weight_cache = kzalloc(sizeof(struct llama_weight_cache), GFP_KERNEL);
    if (model->weight_cache) {
        size_t cache_size = 15ULL * 1024 * 1024 * 1024; /* 15GB */
        int ret = llama_weight_cache_init(model->weight_cache, model->hparams.n_layer, cache_size);
        if (ret < 0) {
            pr_warn("🦙 Llama: Failed to init weight cache, continuing without it\n");
            kfree(model->weight_cache);
            model->weight_cache = NULL;
        }
    }
    
    pr_info("🦙 Llama: Model created - %d layers, %d embd, %d heads\n",
            model->hparams.n_layer, model->hparams.n_embd, model->hparams.n_head);
    
    /* Set global weight cache for GGML operations */
    if (model->weight_cache) {
        extern void ggml_set_weight_cache(struct llama_weight_cache *cache);
        ggml_set_weight_cache(model->weight_cache);
    }
    
    return model;
}

/* Free model */
void llama_model_free(struct llama_model *model) {
    if (!model) return;
    
    /* Clear global weight cache */
    if (model->weight_cache) {
        extern void ggml_set_weight_cache(struct llama_weight_cache *cache);
        ggml_set_weight_cache(NULL);
        llama_weight_cache_free(model->weight_cache);
        kfree(model->weight_cache);
    }
    
    llama_tokenizer_free(&model->tokenizer);
    kfree(model->layers);
    kfree(model);
}

/* Create inference state */
struct llama_state *llama_state_create(struct llama_model *model) {
    struct llama_state *state;
    
    if (!model) return NULL;
    
    state = kzalloc(sizeof(struct llama_state), GFP_KERNEL);
    if (!state) {
        pr_err("🦙 Llama: Failed to allocate state\n");
        return NULL;
    }
    
    state->model = model;
    state->n_vocab = model->hparams.n_vocab;
    
    /* Allocate token buffer */
    state->tokens = kzalloc(model->hparams.n_ctx * sizeof(int32_t), GFP_KERNEL);
    if (!state->tokens) {
        goto err_free_state;
    }
    
    /* Allocate logits buffer - use smaller vocab for kernel testing */
    const int test_vocab = 1000; /* Reduced from 32000 for kernel memory limits */
    state->logits = kzalloc(test_vocab * sizeof(float), GFP_KERNEL);
    if (!state->logits) {
        pr_err("🦙 Llama: Failed to allocate logits buffer (%d floats)\n", test_vocab);
        goto err_free_tokens;
    }
    
    /* Initialize KV cache - full context for CodeLlama 13B */
    const int64_t test_ctx = 2048; /* Full 2K context for code analysis */
    const int64_t n_mem = model->hparams.n_layer * test_ctx;
    const int64_t n_elements = model->hparams.n_embd * n_mem;
    
    pr_info("🦙 Llama: Allocating KV cache for %lld elements (%.2f MB)\n", 
            n_elements * 2, (n_elements * 2 * sizeof(float)) / (1024.0 * 1024.0));
    
    state->cache.k = ggml_new_tensor_1d(model->ctx, GGML_TYPE_F32, n_elements);
    state->cache.v = ggml_new_tensor_1d(model->ctx, GGML_TYPE_F32, n_elements);
    
    if (!state->cache.k || !state->cache.v) {
        pr_err("🦙 Llama: Failed to allocate KV cache\n");
        goto err_free_logits;
    }
    
    state->cache.capacity = test_ctx;
    
    /* Set default sampling parameters */
    state->temperature = 0.8f;
    state->top_p = 0.95f;
    state->top_k = 40;
    
    pr_info("🦙 Llama: State created with %d context length\n", 
            model->hparams.n_ctx);
    
    return state;
    
err_free_logits:
    kfree(state->logits);
err_free_tokens:
    kfree(state->tokens);
err_free_state:
    kfree(state);
    return NULL;
}

/* Free state */
void llama_state_free(struct llama_state *state) {
    if (!state) return;
    
    kfree(state->tokens);
    kfree(state->logits);
    kfree(state);
}

/* Reset state */
void llama_state_reset(struct llama_state *state) {
    if (!state) return;
    
    state->n_tokens = 0;
    state->n_past = 0;
    state->cache.n = 0;
    
    /* Clear KV cache */
    if (state->cache.k && state->cache.k->data) {
        memset(state->cache.k->data, 0, ggml_nbytes(state->cache.k));
    }
    if (state->cache.v && state->cache.v->data) {
        memset(state->cache.v->data, 0, ggml_nbytes(state->cache.v));
    }
}

/* Multi-head attention mechanism */
static struct ggml_tensor *llama_attention(
    struct ggml_context *ctx,
    struct llama_model *model,
    struct llama_state *state,
    struct ggml_tensor *input,
    int layer_idx) {
    
    struct llama_layer *layer = &model->layers[layer_idx];
    const int n_embd = model->hparams.n_embd;
    const int n_head = model->hparams.n_head;
    const int n_head_kv = model->hparams.n_head_kv ?: n_head;
    const int head_dim = n_embd / n_head;
    
    /* Check if we have the required tensors */
    if (!layer->wq || !layer->wk || !layer->wv || !layer->wo) {
        pr_debug("🦙 Llama: Layer %d missing attention weights, using passthrough\n", layer_idx);
        return input;
    }
    
    /* Compute Q, K, V projections */
    pr_info("🦙 Llama: Computing attention for layer %d\n", layer_idx);
    pr_info("🦙 Llama:   input shape: [%lld, %lld]\n", input->ne[0], input->ne[1]);
    pr_info("🦙 Llama:   wq shape: [%lld, %lld]\n", layer->wq->ne[0], layer->wq->ne[1]);
    
    struct ggml_tensor *q = ggml_mul_mat(ctx, layer->wq, input);
    if (!q) {
        pr_err("🦙 Llama: Failed to compute Q projection\n");
        return NULL;
    }
    
    struct ggml_tensor *k = ggml_mul_mat(ctx, layer->wk, input);
    if (!k) {
        pr_err("🦙 Llama: Failed to compute K projection\n");
        return NULL;
    }
    
    struct ggml_tensor *v = ggml_mul_mat(ctx, layer->wv, input);
    if (!v) {
        pr_err("🦙 Llama: Failed to compute V projection\n");
        return NULL;
    }
    
    /* Reshape for multi-head attention */
    /* Q: [n_embd] -> [n_head, head_dim] */
    /* K, V: [n_embd] -> [n_head_kv, head_dim] */
    
    /* Apply RoPE (Rotary Position Embeddings) */
    const int n_past = state->n_past;
    const int rope_dims = model->hparams.n_rot ?: n_embd;
    
    pr_info("🦙 Llama: Applying RoPE with n_past=%d, rope_dims=%d\n", n_past, rope_dims);
    
    q = ggml_rope(ctx, q, n_past, rope_dims, 0);
    if (!q) {
        pr_err("🦙 Llama: RoPE failed for Q\n");
        return NULL;
    }
    
    k = ggml_rope(ctx, k, n_past, rope_dims, 0);
    if (!k) {
        pr_err("🦙 Llama: RoPE failed for K\n");
        return NULL;
    }
    
    /* Update KV cache */
    if (state->cache.k && state->cache.v) {
        /* Store current K and V in cache at position n_past */
        /* For simplicity, we'll implement a basic cache update */
        /* In production, this would involve proper tensor views and copies */
        state->cache.n = n_past + 1;
    }
    
    /* Compute attention scores: Q @ K^T / sqrt(head_dim) */
    /* Q is [hidden, seq_len], K is [hidden, seq_len] */
    /* We need Q^T @ K = [seq_len, seq_len] */
    pr_info("🦙 Llama: Computing attention scores, Q shape: [%lld,%lld], K shape: [%lld,%lld]\n",
            q->ne[0], q->ne[1], k->ne[0], k->ne[1]);
    
    /* Transpose Q and K to get [seq_len, hidden] */
    struct ggml_tensor *q_t = ggml_transpose(ctx, q);
    if (!q_t) {
        pr_err("🦙 Llama: Failed to transpose Q\n");
        return NULL;
    }
    
    struct ggml_tensor *k_t = ggml_transpose(ctx, k);
    if (!k_t) {
        pr_err("🦙 Llama: Failed to transpose K\n");
        return NULL;
    }
    
    /* Now compute Q_t @ K_t^T which gives us [seq_len, seq_len] */
    /* q_t is [6, 5120], k_t is [6, 5120] */
    /* ggml_mul_mat(A, B) computes A @ B^T, so this gives us q_t @ k_t^T = [6, 6] */
    struct ggml_tensor *scores = ggml_mul_mat(ctx, k_t, q_t);
    if (!scores) {
        pr_err("🦙 Llama: Failed to compute attention scores\n");
        return NULL;
    }
    
    /* Scale by 1/sqrt(head_dim) */
    /* Use integer approximation for kernel space */
    float scale = 1.0f;
    if (head_dim == 64) {
        scale = 0.125f; /* 1/sqrt(64) = 1/8 = 0.125 */
    } else if (head_dim == 128) {
        scale = 0.0884f; /* 1/sqrt(128) ≈ 0.0884 */
    } else {
        /* General case - use approximation */
        scale = 1.0f / (float)head_dim;
    }
    scores = ggml_scale(ctx, scores, scale);
    if (!scores) {
        pr_err("🦙 Llama: Failed to scale attention scores\n");
        return NULL;
    }
    
    /* Apply softmax */
    scores = ggml_soft_max(ctx, scores);
    if (!scores) {
        pr_err("🦙 Llama: Failed to apply softmax\n");
        return NULL;
    }
    
    /* Apply attention to values: scores @ V */
    pr_info("🦙 Llama: Applying attention - scores shape: [%lld,%lld], V shape: [%lld,%lld]\n",
            scores->ne[0], scores->ne[1], v->ne[0], v->ne[1]);
    
    /* scores is [seq_len, seq_len], V is [hidden, seq_len] */
    /* We need scores @ V^T to get [seq_len, hidden] */
    /* Since ggml_mul_mat computes A @ B^T, we pass (v, scores) to get scores @ v^T */
    struct ggml_tensor *attn_output = ggml_mul_mat(ctx, v, scores);
    if (!attn_output) {
        pr_err("🦙 Llama: Failed to apply attention to values\n");
        return NULL;
    }
    
    /* Now we have [seq_len, hidden], but we need [hidden, seq_len] for the output projection */
    attn_output = ggml_transpose(ctx, attn_output);
    if (!attn_output) {
        pr_err("🦙 Llama: Failed to transpose attention output\n");
        return NULL;
    }
    
    /* Project back to embedding dimension */
    attn_output = ggml_mul_mat(ctx, layer->wo, attn_output);
    if (!attn_output) {
        pr_err("🦙 Llama: Failed to project attention output\n");
        return NULL;
    }
    
    return attn_output;
}

/* Forward pass through one transformer layer */
static struct ggml_tensor *llama_layer_forward(
    struct ggml_context *ctx,
    struct llama_model *model,
    struct llama_state *state,
    struct ggml_tensor *input,
    int layer_idx) {
    
    struct llama_layer *layer = &model->layers[layer_idx];
    struct ggml_tensor *cur = input;
    
    /* Input layer norm */
    cur = ggml_rms_norm(ctx, cur, 1e-5f);
    if (!cur) {
        pr_err("🦙 Llama: RMS norm failed in layer %d\n", layer_idx);
        return NULL;
    }
    
    if (layer->attention_norm) {
        cur = ggml_mul(ctx, cur, layer->attention_norm);
        if (!cur) {
            pr_err("🦙 Llama: Attention norm mul failed in layer %d\n", layer_idx);
            return NULL;
        }
    }
    
    /* Self-attention */
    struct ggml_tensor *attn_out = llama_attention(ctx, model, state, cur, layer_idx);
    if (!attn_out) {
        pr_err("🦙 Llama: Attention failed in layer %d\n", layer_idx);
        return NULL;
    }
    
    /* Add residual */
    cur = ggml_add(ctx, input, attn_out);
    if (!cur) {
        pr_err("🦙 Llama: Residual add failed in layer %d\n", layer_idx);
        return NULL;
    }
    
    /* FFN */
    struct ggml_tensor *ffn_input = cur;
    cur = ggml_rms_norm(ctx, cur, 1e-5f);
    if (layer->ffn_norm) {
        cur = ggml_mul(ctx, cur, layer->ffn_norm);
    }
    
    /* FFN layers - skip if weights not loaded */
    if (layer->w1 && layer->w2 && layer->w3) {
        /* Debug: print tensor shapes */
        pr_info("🦙 FFN input cur: [%lld, %lld]\n", cur->ne[0], cur->ne[1]);
        pr_info("🦙 FFN w1 (gate): [%lld, %lld]\n", layer->w1->ne[0], layer->w1->ne[1]);
        pr_info("🦙 FFN w3 (up): [%lld, %lld]\n", layer->w3->ne[0], layer->w3->ne[1]);
        pr_info("🦙 FFN w2 (down): [%lld, %lld]\n", layer->w2->ne[0], layer->w2->ne[1]);
        
        /* Simplified approach: use the correct operation order
         * ggml_mul_mat(A, B) computes A @ B^T
         * 
         * We have:
         * - cur: [5120, seq_len] 
         * - w1/w3: [5120, 13824]
         * 
         * We want: cur^T @ w1^T = [seq_len, 5120] @ [13824, 5120]^T = [seq_len, 13824]
         * Then transpose to get [13824, seq_len]
         * 
         * Using ggml_mul_mat(w1, cur) = w1 @ cur^T = [5120, 13824] @ [seq_len, 5120]
         * This needs w1.ne[0] == cur.ne[0] = 5120 == 5120 ✓
         * Result: [cur.ne[1], w1.ne[1]] = [seq_len, 13824]
         */
        
        /* Gate and up projections */
        struct ggml_tensor *gate = ggml_mul_mat(ctx, layer->w1, cur);  /* [seq_len, 13824] */
        if (!gate) {
            pr_err("🦙 FFN: gate mul_mat failed\n");
            return NULL;
        }
        
        struct ggml_tensor *up = ggml_mul_mat(ctx, layer->w3, cur);    /* [seq_len, 13824] */
        if (!up) {
            pr_err("🦙 FFN: up mul_mat failed\n");
            return NULL;
        }
        
        gate = ggml_silu(ctx, gate);
        if (!gate) {
            pr_err("🦙 FFN: silu failed\n");
            return NULL;
        }
        cur = ggml_mul(ctx, gate, up);  /* [seq_len, 13824] */
        if (!cur) {
            pr_err("🦙 FFN: gate*up mul failed\n");
            return NULL;
        }
        
        /* For down projection: cur is [seq_len, 13824], w2 is [13824, 5120]
         * We want: cur @ w2^T = [seq_len, 13824] @ [5120, 13824]^T = [seq_len, 5120]
         * 
         * Using ggml_mul_mat(cur, w2) = cur @ w2^T = [seq_len, 13824] @ [5120, 13824]
         * This needs cur.ne[0] == w2.ne[0]... but cur.ne[0] = seq_len and w2.ne[0] = 13824
         * 
         * Actually, we need to think about this differently.
         * The result of gate*up is [seq_len, 13824]
         * We need to project this down to [seq_len, 5120]
         * 
         * Standard: result = hidden @ W2^T where W2 is [5120, 13824]
         * So we want: [seq_len, 13824] @ [13824, 5120] = [seq_len, 5120]
         * 
         * With ggml_mul_mat(A, B) = A @ B^T:
         * We need A @ B^T = [seq_len, 13824] @ [13824, 5120]
         * So B should be [5120, 13824]^T = [13824, 5120]
         * But w2 is already [13824, 5120]!
         * 
         * Wait, let me check the actual dimensions...
         */
        pr_info("🦙 FFN down: cur is [%lld, %lld], w2 is [%lld, %lld]\n",
                cur->ne[0], cur->ne[1], layer->w2->ne[0], layer->w2->ne[1]);
        
        /* Let's try a different approach - transpose cur first */
        struct ggml_tensor *cur_t = ggml_transpose(ctx, cur);  /* [13824, seq_len] */
        if (!cur_t) {
            pr_err("🦙 FFN: cur transpose failed\n");
            return NULL;
        }
        
        /* Now: ggml_mul_mat(w2, cur_t) = [13824, 5120] @ [13824, seq_len]^T = [13824, 5120] @ [seq_len, 13824]
         * This needs w2.ne[0] == cur_t.ne[0] = 13824 == 13824 ✓
         * Result: [cur_t.ne[1], w2.ne[1]] = [seq_len, 5120]
         */
        cur = ggml_mul_mat(ctx, layer->w2, cur_t);
        if (!cur) {
            pr_err("🦙 FFN: down projection failed\n");
            return NULL;
        }
    }
    
    /* Add residual */
    cur = ggml_add(ctx, ffn_input, cur);
    
    return cur;
}

/* Run forward pass */
int llama_eval(struct llama_state *state,
               const int32_t *tokens,
               int n_tokens,
               int n_past) {
    
    struct llama_model *model = state->model;
    struct ggml_context *ctx = model->ctx;
    
    if (!model || !ctx || !tokens || n_tokens <= 0) {
        return -EINVAL;
    }
    
    /* Create embeddings */
    struct ggml_tensor *embd = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, n_tokens);
    if (!embd) {
        pr_err("🦙 Llama: Failed to create embedding indices tensor!\n");
        return -EINVAL;
    }
    memcpy(embd->data, tokens, n_tokens * sizeof(int32_t));
    
    pr_info("🦙 Llama: Getting embeddings for %d tokens, tok_embeddings=%p\n", 
            n_tokens, model->tok_embeddings);
    
    struct ggml_tensor *cur = ggml_get_rows(ctx, model->tok_embeddings, embd);
    if (!cur) {
        pr_err("🦙 Llama: Failed to get embeddings! tok_embeddings=%p, embd=%p\n",
               model->tok_embeddings, embd);
        return -EINVAL;
    }
    
    /* Run through transformer layers */
    for (int i = 0; i < model->hparams.n_layer; i++) {
        if (i % 10 == 0) {
            pr_info("🦙 Llama: Processing layer %d/%d, nodes: %d\n", 
                    i, model->hparams.n_layer, ctx->n_objects);
        }
        cur = llama_layer_forward(ctx, model, state, cur, i);
        if (!cur) {
            pr_err("🦙 Llama: Layer %d forward pass failed!\n", i);
            return -EINVAL;
        }
    }
    
    /* Final norm */
    cur = ggml_rms_norm(ctx, cur, 1e-5f);
    if (model->norm) {
        cur = ggml_mul(ctx, cur, model->norm);
    }
    
    /* Output projection */
    if (model->output) {
        cur = ggml_mul_mat(ctx, model->output, cur);
    }
    
    /* Build and execute the computation graph */
    struct ggml_cgraph *gf = ggml_build_forward(cur);
    if (!gf) {
        pr_err("🦙 Llama: Failed to build computation graph!\n");
        return -EINVAL;
    }
    
    pr_info("🦙 Llama: Executing computation graph...\n");
    ggml_graph_compute(ctx, gf);
    
    /* Copy logits */
    if (cur->ne[0] == model->hparams.n_vocab) {
        memcpy(state->logits, cur->data, 
               model->hparams.n_vocab * sizeof(float));
    }
    
    state->n_tokens = n_tokens;
    state->n_past = n_past + n_tokens;
    
    return 0;
}

/* Sample next token */
int32_t llama_sample_token(struct llama_state *state) {
    if (!state || !state->logits || !state->model) {
        return -1;
    }
    
    const int n_vocab = state->model->hparams.n_vocab;
    float *logits = state->logits;
    
    /* Find token with highest logit (greedy sampling) */
    int32_t best_token = 0;
    float best_logit = logits[0];
    
    kernel_fpu_begin();
    for (int i = 1; i < n_vocab; i++) {
        if (logits[i] > best_logit) {
            best_logit = logits[i];
            best_token = i;
        }
    }
    kernel_fpu_end();
    
    /* Debug: print selected token */
    pr_info("🦙 Llama: Sampled token %d (logit=%.3f)\n", 
            best_token, best_logit);
    
    return best_token;
}

/* High-level generation function */
int llama_generate(struct llama_state *state,
                   const char *prompt,
                   char *output,
                   int max_length,
                   int max_tokens) {
    
    int32_t tokens[512];
    int n_tokens;
    int n_generated = 0;
    ktime_t start_time, end_time;
    s64 elapsed_ms;
    
    if (!state || !prompt || !output || max_length <= 0) {
        return -EINVAL;
    }
    
    /* Track request stats */
    extern struct llamux_stats llamux_perf_stats;
    atomic64_inc(&llamux_perf_stats.total_requests);
    
    /* Start timing */
    start_time = ktime_get();
    atomic64_set(&llamux_perf_stats.last_inference_start, ktime_to_ms(start_time));
    
    /* Reset state */
    llama_state_reset(state);
    
    /* Tokenize prompt using model's tokenizer */
    n_tokens = llama_tokenize(&state->model->tokenizer, prompt, tokens, 512);
    if (n_tokens <= 0) {
        pr_err("🦙 Llama: Failed to tokenize prompt\n");
        atomic64_inc(&llamux_perf_stats.failed_requests);
        return -1;
    }
    
    pr_info("🦙 Llama: Tokenized prompt into %d tokens\n", n_tokens);
    
    /* Note: BOS token is already added by llama_tokenize */
    
    /* Evaluate prompt */
    int ret = llama_eval(state, tokens, n_tokens, 0);
    if (ret < 0) {
        pr_err("🦙 Llama: Eval failed with error %d\n", ret);
        atomic64_inc(&llamux_perf_stats.failed_requests);
        return -1;
    }
    
    /* Generate tokens */
    int32_t generated_tokens[256];
    int n_gen = 0;
    
    for (int i = 0; i < max_tokens && n_gen < 256; i++) {
        /* Sample next token */
        int32_t next_token = llama_sample_token(state);
        if (next_token < 0) {
            break;
        }
        
        /* Check for EOS */
        if (next_token == 2) { /* EOS token */
            break;
        }
        
        generated_tokens[n_gen++] = next_token;
        
        /* Reset context to reuse memory for next token
         * This is a hack but necessary to avoid running out of nodes
         * We keep the model weights but reset the computation graph
         */
        if (state->model && state->model->ctx) {
            /* Save current object count */
            int saved_objects = state->model->ctx->n_objects;
            
            /* Find where temporary tensors start (after model weights) */
            /* Assume first ~2000 objects are model weights */
            if (saved_objects > 2000) {
                state->model->ctx->n_objects = 2000;
                state->model->ctx->mem_used = state->model->ctx->mem_used / 2; /* Rough estimate */
            }
        }
        
        /* Evaluate new token */
        ret = llama_eval(state, &next_token, 1, state->n_past);
        if (ret < 0) {
            break;
        }
        
        n_generated++;
    }
    
    /* Detokenize response using model's tokenizer */
    llama_detokenize(&state->model->tokenizer, generated_tokens, n_gen, output, max_length);
    
    /* Debug: show first few token IDs */
    pr_info("🦙 Llama: First 10 tokens: %d %d %d %d %d %d %d %d %d %d\n",
            n_gen > 0 ? generated_tokens[0] : -1,
            n_gen > 1 ? generated_tokens[1] : -1,
            n_gen > 2 ? generated_tokens[2] : -1,
            n_gen > 3 ? generated_tokens[3] : -1,
            n_gen > 4 ? generated_tokens[4] : -1,
            n_gen > 5 ? generated_tokens[5] : -1,
            n_gen > 6 ? generated_tokens[6] : -1,
            n_gen > 7 ? generated_tokens[7] : -1,
            n_gen > 8 ? generated_tokens[8] : -1,
            n_gen > 9 ? generated_tokens[9] : -1);
    
    pr_info("🦙 Llama: Generated %d tokens: %s\n", n_generated, output);
    
    /* Calculate performance stats */
    end_time = ktime_get();
    elapsed_ms = ktime_ms_delta(end_time, start_time);
    
    /* Update stats */
    atomic64_add(n_generated, &llamux_perf_stats.total_tokens_generated);
    atomic64_add(elapsed_ms, &llamux_perf_stats.total_inference_time_ms);
    atomic_set(&llamux_perf_stats.last_batch_tokens, n_generated);
    
    /* Calculate tokens per second for this batch */
    if (elapsed_ms > 0) {
        int tokens_per_sec = (n_generated * 1000) / elapsed_ms;
        atomic_set(&llamux_perf_stats.current_tokens_per_sec, tokens_per_sec);
        pr_info("🦙 Llama: Performance: %d tokens in %lld ms = %d tokens/sec\n",
                n_generated, elapsed_ms, tokens_per_sec);
    }
    
    /* Update peak memory if needed */
    if (state->model->ctx) {
        u64 current_mem = state->model->ctx->mem_used;
        u64 peak = atomic64_read(&llamux_perf_stats.peak_memory_used);
        if (current_mem > peak) {
            atomic64_set(&llamux_perf_stats.peak_memory_used, current_mem);
        }
    }
    
    return n_generated;
}

/* Print model information */
void llama_print_model_info(struct llama_model *model) {
    if (!model) return;
    
    pr_info("🦙 Llama Model Information:\n");
    pr_info("  Architecture: LLaMA\n");
    pr_info("  Layers: %d\n", model->hparams.n_layer);
    pr_info("  Embedding: %d\n", model->hparams.n_embd);
    pr_info("  Heads: %d\n", model->hparams.n_head);
    pr_info("  Context: %d tokens\n", model->hparams.n_ctx);
    pr_info("  Vocabulary: %d tokens\n", model->hparams.n_vocab);
    pr_info("  Feed Forward: %d\n", model->hparams.n_ff);
}