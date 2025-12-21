// 01_tokenizer.c - Tokenizer implementation
#include <stdio.h>
#include <stdlib.h>
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
    printf("Tokenizer started\n");
    printf("Data structures ready:\n");
    printf("  - tokens[]\n");
    printf("  - pair_table[]\n");
    printf("  - merges[]\n");
    return 0;
}
