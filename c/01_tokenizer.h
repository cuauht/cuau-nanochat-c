// 01_tokenizer.h - Tokenizer declarations
#include <stdint.h>

// ============================================================
// DATA STRUCTURES
// ============================================================

// A pair of adjacent tokens
typedef struct {
    uint32_t left;
    uint32_t right;
} Pair;

// Hash table entry for counting pairs
typedef struct {
    Pair pair;
    uint32_t count;
    uint8_t used;
} PairEntry;

// Merge record (which pair became which token)
typedef struct {
    Pair pair;
    uint32_t new_token;
} Merge;

// ============================================================
// GLOBAL STATE
// ============================================================

// The token sequence (starts as bytes, gets merged)
extern uint32_t* tokens;
extern uint32_t tokens_len;

// Hash table for pair counts
extern PairEntry* pair_table;
extern uint32_t pair_table_size;

// List of merges performed
extern Merge* merges;
extern uint32_t merges_len;

// Vocabulary size target
extern uint32_t vocab_size;

// ============================================================
// FUNCTIONS
// ============================================================

int tokenizer_init(void);
