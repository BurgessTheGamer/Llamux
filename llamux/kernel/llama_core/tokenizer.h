/*
 * Simple Tokenizer for Llamux
 * 
 * Basic tokenization for LLM input/output in kernel space.
 * This is a minimal implementation for proof of concept.
 */

#ifndef _LLAMUX_TOKENIZER_H
#define _LLAMUX_TOKENIZER_H

#include <linux/types.h>

#define MAX_TOKEN_LENGTH    64
#define MAX_TOKENS          2048
#define VOCAB_SIZE          32000  /* TinyLlama vocab size */

/* Token structure */
struct llama_token {
    int32_t id;
    char text[MAX_TOKEN_LENGTH];
    float score;
};

/* Vocabulary */
struct llama_vocab {
    int32_t n_vocab;
    struct llama_token *tokens;
    
    /* Special tokens */
    int32_t bos_token_id;  /* Beginning of sequence */
    int32_t eos_token_id;  /* End of sequence */
    int32_t pad_token_id;  /* Padding */
    int32_t unk_token_id;  /* Unknown */
};

/* Tokenizer context */
struct llama_tokenizer {
    struct llama_vocab *vocab;
    bool initialized;
};

/* Function prototypes */
int llama_tokenizer_init(struct llama_tokenizer *tokenizer);
int llama_tokenizer_init_from_gguf(struct llama_tokenizer *tokenizer, 
                                   char **vocab_tokens, u32 vocab_size,
                                   u32 bos_id, u32 eos_id, u32 unk_id, u32 pad_id);
void llama_tokenizer_free(struct llama_tokenizer *tokenizer);

/* Tokenization functions */
int llama_tokenize(struct llama_tokenizer *tokenizer,
                   const char *text,
                   int32_t *tokens,
                   int max_tokens);

int llama_detokenize(struct llama_tokenizer *tokenizer,
                     const int32_t *tokens,
                     int n_tokens,
                     char *text,
                     int max_length);

/* Simple tokenization for testing */
int llama_tokenize_simple(const char *text, int32_t *tokens, int max_tokens);
int llama_detokenize_simple(const int32_t *tokens, int n_tokens, char *text, int max_length);

#endif /* _LLAMUX_TOKENIZER_H */