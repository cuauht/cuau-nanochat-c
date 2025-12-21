// 01_tokenizer.c - Tokenizer implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "01_tokenizer.h"

// ============================================================
// GLOBAL STATE
// ============================================================

// Array of token IDs (starts as bytes 0-255, new tokens added as we merge)
uint32_t* tokens = NULL;

// How many tokens in the array
uint32_t tokens_len = 0;

// Hash table to count how many times each pair appears
PairEntry* pair_table = NULL;

// Size of the hash table
uint32_t pair_table_size = 0;

// List of merges we performed (pair -> new token)
Merge* merges = NULL;

// How many merges we did
uint32_t merges_len = 0;

// Target vocabulary size (256 bytes + number of merges)
uint32_t vocab_size = 256;

// ============================================================
// FUNCTIONS
// ============================================================

int tokenizer_init(void) {
    printf("    C: Tokenizer initialized\n");
    printf("    C: tokens[] array ready (starts empty)\n");
    return 0;
}

// Load text and convert to bytes (initial tokens)
void load_text(const char* text, size_t len) {
    uint32_t old_len = tokens_len;
    
    // Reallocate tokens array to fit new bytes
    tokens = realloc(tokens, (tokens_len + len) * sizeof(uint32_t));
    
    // Convert each byte to a token (0-255)
    for (size_t i = 0; i < len; i++) {
        tokens[tokens_len + i] = (uint8_t)text[i];
    }
    
    tokens_len += (uint32_t)len;
    
    // Print progress every time
    printf("           C: Added %zu bytes -> tokens[%u..%u]\n", len, old_len, tokens_len - 1);
}
