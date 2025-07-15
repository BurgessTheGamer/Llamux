/*
 * Simple Tokenizer Implementation for Llamux
 * 
 * This is a very basic tokenizer for demonstration purposes.
 * A real implementation would load the actual TinyLlama vocabulary.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include "tokenizer.h"

/* Simple vocabulary for testing */
static const char *simple_vocab[] = {
    "<unk>",    /* 0 */
    "<s>",      /* 1 - BOS */
    "</s>",     /* 2 - EOS */
    "<pad>",    /* 3 - PAD */
    " ",        /* 4 - space */
    "the",      /* 5 */
    "a",        /* 6 */
    "is",       /* 7 */
    "in",       /* 8 */
    "to",       /* 9 */
    "of",       /* 10 */
    "and",      /* 11 */
    "for",      /* 12 */
    "on",       /* 13 */
    "with",     /* 14 */
    "as",       /* 15 */
    "by",       /* 16 */
    "at",       /* 17 */
    "from",     /* 18 */
    "that",     /* 19 */
    "it",       /* 20 */
    "this",     /* 21 */
    "was",      /* 22 */
    "are",      /* 23 */
    "be",       /* 24 */
    "have",     /* 25 */
    "has",      /* 26 */
    "had",      /* 27 */
    "do",       /* 28 */
    "does",     /* 29 */
    "did",      /* 30 */
    "will",     /* 31 */
    "would",    /* 32 */
    "could",    /* 33 */
    "should",   /* 34 */
    "may",      /* 35 */
    "might",    /* 36 */
    "must",     /* 37 */
    "can",      /* 38 */
    "system",   /* 39 */
    "process",  /* 40 */
    "memory",   /* 41 */
    "kernel",   /* 42 */
    "linux",    /* 43 */
    "llamux",   /* 44 */
    "llama",    /* 45 */
    "file",     /* 46 */
    "user",     /* 47 */
    "time",     /* 48 */
    "data",     /* 49 */
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",  /* 50-59 */
    ".", ",", "!", "?", ":", ";", "'", "\"", "-", "_", /* 60-69 */
};

#define SIMPLE_VOCAB_SIZE (sizeof(simple_vocab) / sizeof(simple_vocab[0]))

/* Initialize tokenizer */
int llama_tokenizer_init(struct llama_tokenizer *tokenizer) {
    if (!tokenizer) {
        return -EINVAL;
    }
    
    /* Allocate vocabulary */
    tokenizer->vocab = kzalloc(sizeof(struct llama_vocab), GFP_KERNEL);
    if (!tokenizer->vocab) {
        return -ENOMEM;
    }
    
    /* For now, use simple vocabulary */
    tokenizer->vocab->n_vocab = SIMPLE_VOCAB_SIZE;
    tokenizer->vocab->bos_token_id = 1;
    tokenizer->vocab->eos_token_id = 2;
    tokenizer->vocab->pad_token_id = 3;
    tokenizer->vocab->unk_token_id = 0;
    
    /* Allocate token array */
    tokenizer->vocab->tokens = kzalloc(
        SIMPLE_VOCAB_SIZE * sizeof(struct llama_token), GFP_KERNEL);
    if (!tokenizer->vocab->tokens) {
        kfree(tokenizer->vocab);
        return -ENOMEM;
    }
    
    /* Initialize simple vocabulary */
    for (int i = 0; i < SIMPLE_VOCAB_SIZE; i++) {
        tokenizer->vocab->tokens[i].id = i;
        strncpy(tokenizer->vocab->tokens[i].text, simple_vocab[i], 
                MAX_TOKEN_LENGTH - 1);
        tokenizer->vocab->tokens[i].score = 1.0f;
    }
    
    tokenizer->initialized = true;
    
    pr_info("🦙 Tokenizer: Initialized with %d tokens\n", 
            tokenizer->vocab->n_vocab);
    
    return 0;
}

/* Free tokenizer */
void llama_tokenizer_free(struct llama_tokenizer *tokenizer) {
    if (!tokenizer || !tokenizer->initialized) {
        return;
    }
    
    if (tokenizer->vocab) {
        kfree(tokenizer->vocab->tokens);
        kfree(tokenizer->vocab);
        tokenizer->vocab = NULL;
    }
    
    tokenizer->initialized = false;
}

/* Simple word-based tokenization */
int llama_tokenize_simple(const char *text, int32_t *tokens, int max_tokens) {
    int n_tokens = 0;
    const char *p = text;
    char word[MAX_TOKEN_LENGTH];
    int word_len = 0;
    
    if (!text || !tokens || max_tokens <= 0) {
        return -EINVAL;
    }
    
    /* Add BOS token */
    if (n_tokens < max_tokens) {
        tokens[n_tokens++] = 1;  /* <s> */
    }
    
    /* Simple word splitting */
    while (*p && n_tokens < max_tokens - 1) {
        if (isspace(*p)) {
            if (word_len > 0) {
                /* End of word */
                word[word_len] = '\0';
                
                /* Find token in vocabulary */
                int token_id = 0;  /* <unk> by default */
                for (int i = 0; i < SIMPLE_VOCAB_SIZE; i++) {
                    if (strcmp(word, simple_vocab[i]) == 0) {
                        token_id = i;
                        break;
                    }
                }
                
                tokens[n_tokens++] = token_id;
                word_len = 0;
                
                /* Add space token if not at end */
                if (n_tokens < max_tokens - 1 && *(p + 1) != '\0') {
                    tokens[n_tokens++] = 4;  /* space */
                }
            }
            p++;
        } else {
            /* Add character to current word */
            if (word_len < MAX_TOKEN_LENGTH - 1) {
                word[word_len++] = tolower(*p);
            }
            p++;
        }
    }
    
    /* Handle last word */
    if (word_len > 0 && n_tokens < max_tokens - 1) {
        word[word_len] = '\0';
        int token_id = 0;
        for (int i = 0; i < SIMPLE_VOCAB_SIZE; i++) {
            if (strcmp(word, simple_vocab[i]) == 0) {
                token_id = i;
                break;
            }
        }
        tokens[n_tokens++] = token_id;
    }
    
    /* Add EOS token */
    if (n_tokens < max_tokens) {
        tokens[n_tokens++] = 2;  /* </s> */
    }
    
    return n_tokens;
}

/* Simple detokenization */
int llama_detokenize_simple(const int32_t *tokens, int n_tokens, 
                           char *text, int max_length) {
    int pos = 0;
    
    if (!tokens || !text || max_length <= 0) {
        return -EINVAL;
    }
    
    text[0] = '\0';
    
    for (int i = 0; i < n_tokens; i++) {
        int32_t token_id = tokens[i];
        
        /* Skip special tokens in output */
        if (token_id == 1 || token_id == 2 || token_id == 3) {
            continue;
        }
        
        if (token_id >= 0 && token_id < SIMPLE_VOCAB_SIZE) {
            const char *token_text = simple_vocab[token_id];
            int token_len = strlen(token_text);
            
            if (pos + token_len < max_length - 1) {
                strcpy(text + pos, token_text);
                pos += token_len;
            } else {
                break;
            }
        }
    }
    
    text[pos] = '\0';
    return pos;
}

/* Full tokenization using vocabulary */
int llama_tokenize(struct llama_tokenizer *tokenizer,
                   const char *text,
                   int32_t *tokens,
                   int max_tokens) {
    if (!tokenizer || !tokenizer->initialized) {
        /* Fall back to simple tokenization */
        return llama_tokenize_simple(text, tokens, max_tokens);
    }
    
    /* TODO: Implement BPE or other proper tokenization */
    return llama_tokenize_simple(text, tokens, max_tokens);
}

/* Full detokenization using vocabulary */
int llama_detokenize(struct llama_tokenizer *tokenizer,
                     const int32_t *tokens,
                     int n_tokens,
                     char *text,
                     int max_length) {
    if (!tokenizer || !tokenizer->initialized) {
        /* Fall back to simple detokenization */
        return llama_detokenize_simple(tokens, n_tokens, text, max_length);
    }
    
    int pos = 0;
    text[0] = '\0';
    
    for (int i = 0; i < n_tokens; i++) {
        int32_t token_id = tokens[i];
        
        /* Skip special tokens */
        if (token_id == tokenizer->vocab->bos_token_id ||
            token_id == tokenizer->vocab->eos_token_id ||
            token_id == tokenizer->vocab->pad_token_id) {
            continue;
        }
        
        if (token_id >= 0 && token_id < tokenizer->vocab->n_vocab) {
            const char *token_text = tokenizer->vocab->tokens[token_id].text;
            int token_len = strlen(token_text);
            
            if (pos + token_len < max_length - 1) {
                strcpy(text + pos, token_text);
                pos += token_len;
            } else {
                break;
            }
        }
    }
    
    text[pos] = '\0';
    return pos;
}