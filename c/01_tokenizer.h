// 01_tokenizer.h - Tokenizer declarations
#include <stdint.h>

// ============================================================
// DATA STRUCTURES
// ============================================================

// A pair of two adjacent tokens
typedef struct {
    uint32_t left;   // First token
    uint32_t right;  // Second token
} Pair;

// One entry in the hash table
typedef struct {
    Pair pair;       // The pair we are counting
    uint32_t count;  // How many times this pair appears
    uint8_t used;    // 1 if this slot is used, 0 if empty
} PairEntry;

// One merge operation
typedef struct {
    Pair pair;          // The pair we merged
    uint32_t new_token; // The new token ID it became
} Merge;

// ============================================================
// GLOBAL STATE
// ============================================================

// Array of token IDs (starts as bytes 0-255, new tokens added as we merge)
extern uint32_t* tokens;

// How many tokens in the array
extern uint32_t tokens_len;

// Hash table to count how many times each pair appears
extern PairEntry* pair_table;

// Size of the hash table
extern uint32_t pair_table_size;

// List of merges we performed (pair -> new token)
extern Merge* merges;

// How many merges we did
extern uint32_t merges_len;

// Target vocabulary size (256 bytes + number of merges)
extern uint32_t vocab_size;

// ============================================================
// FUNCTIONS
// ============================================================

int tokenizer_init(void);
