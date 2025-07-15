/*
 * TinyLlama Model Definition for Llamux
 * 
 * Model structure and inference functions for kernel-space LLM.
 * Based on TinyLlama-1.1B architecture.
 */

#ifndef _LLAMUX_LLAMA_MODEL_H
#define _LLAMUX_LLAMA_MODEL_H

#include <linux/types.h>
#include "ggml_kernel.h"
#include "tokenizer.h"
#include "weight_cache.h"

/* Forward declaration */
struct gguf_model;

/* Model hyperparameters (TinyLlama-1.1B) */
#define LLAMA_N_VOCAB      32000
#define LLAMA_N_CTX        2048    /* max context length */
#define LLAMA_N_EMBD       2048    /* embedding dimension */
#define LLAMA_N_HEAD       32      /* number of heads */
#define LLAMA_N_LAYER      22      /* number of layers */
#define LLAMA_N_FF         5632    /* feedforward dimension */

/* RoPE parameters */
#define LLAMA_ROPE_DIM     64      /* rotary embedding dimension */
#define LLAMA_ROPE_THETA   10000.0f

/* Model parameters */
struct llama_hparams {
    int32_t n_vocab;
    int32_t n_ctx;
    int32_t n_embd;
    int32_t n_head;
    int32_t n_head_kv; /* number of key-value heads (for GQA) */
    int32_t n_layer;
    int32_t n_ff;
    int32_t n_rot;     /* rotary embedding dimension */
    
    float f_norm_eps;  /* layer norm epsilon */
    float rope_theta;  /* RoPE theta */
};

/* Layer weights */
struct llama_layer {
    /* Attention weights */
    struct ggml_tensor *wq;     /* query projection */
    struct ggml_tensor *wk;     /* key projection */
    struct ggml_tensor *wv;     /* value projection */
    struct ggml_tensor *wo;     /* output projection */
    
    /* Feedforward weights */
    struct ggml_tensor *w1;     /* gate projection */
    struct ggml_tensor *w2;     /* down projection */
    struct ggml_tensor *w3;     /* up projection */
    
    /* Normalization */
    struct ggml_tensor *ffn_norm;      /* feedforward norm */
    struct ggml_tensor *attention_norm; /* attention norm */
};

/* Complete model */
struct llama_model {
    struct llama_hparams hparams;
    
    /* Token embeddings */
    struct ggml_tensor *tok_embeddings;
    
    /* Output norm */
    struct ggml_tensor *norm;
    
    /* Output projection */
    struct ggml_tensor *output;
    
    /* Layers */
    struct llama_layer *layers;
    
    /* Context */
    struct ggml_context *ctx;
    
    /* Tokenizer */
    struct llama_tokenizer tokenizer;
    
    /* Weight cache for fast inference */
    struct llama_weight_cache *weight_cache;
};

/* KV cache for inference */
struct llama_kv_cache {
    struct ggml_tensor *k;  /* key cache */
    struct ggml_tensor *v;  /* value cache */
    
    int32_t n;              /* number of tokens in cache */
    int32_t capacity;       /* max capacity */
};

/* Inference state */
struct llama_state {
    struct llama_model *model;
    struct llama_kv_cache cache;
    
    /* Current sequence */
    int32_t *tokens;
    int32_t n_tokens;
    int32_t n_past;         /* number of processed tokens */
    
    /* Logits */
    float *logits;
    int32_t n_vocab;
    
    /* Sampling parameters */
    float temperature;
    float top_p;
    int32_t top_k;
};

/* Model functions */
struct llama_model *llama_model_create(struct ggml_context *ctx);
struct llama_model *llama_model_create_from_gguf(struct ggml_context *ctx, struct gguf_model *gguf);
void llama_model_free(struct llama_model *model);
int llama_model_load_weights(struct llama_model *model, void *data, size_t size);

/* State functions */
struct llama_state *llama_state_create(struct llama_model *model);
void llama_state_free(struct llama_state *state);
void llama_state_reset(struct llama_state *state);

/* Inference functions */
int llama_eval(struct llama_state *state,
               const int32_t *tokens,
               int32_t n_tokens,
               int32_t n_past);

int32_t llama_sample_token(struct llama_state *state);

/* High-level generation */
int llama_generate(struct llama_state *state,
                   const char *prompt,
                   char *output,
                   int max_length,
                   int max_tokens);

/* Utility functions */
void llama_print_model_info(struct llama_model *model);

#endif /* _LLAMUX_LLAMA_MODEL_H */