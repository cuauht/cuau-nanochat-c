// 01_tokenizer.c - Tokenizer implementation
#include <stdio.h>
#include <stdlib.h>
#include "01_tokenizer.h"

// ============================================================
// GLOBAL STATE (defined here)
// ============================================================

// The token sequence (starts as bytes 0-255, grows with merges)
uint32_t* tokens = NULL;
uint32_t tokens_len = 0;

// Hash table for pair counts
// Key: pair (left, right)
// Value: count of occurrences
PairEntry* pair_table = NULL;
uint32_t pair_table_size = 0;

// List of merges performed in order
// Each merge: (pair) -> new_token
Merge* merges = NULL;
uint32_t merges_len = 0;

// Target vocabulary size (256 base + num_merges)
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
