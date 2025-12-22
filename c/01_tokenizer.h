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
// NEW: CHUNK - A unique piece of text with its count
// ============================================================
// 
// Instead of storing " the" 50,000 times, we store it ONCE
// with count=50000. This makes counting pairs much faster!
//
typedef struct {
    uint32_t* tokens;   // The bytes/tokens in this chunk
    uint32_t len;       // How many tokens in this chunk
    uint32_t count;     // How many times this chunk appears in the data
} Chunk;

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
// NEW: CHUNK STORAGE (the smart way!)
// ============================================================

// Array of unique chunks
extern Chunk* chunks;

// How many unique chunks we have
extern uint32_t num_chunks;

// Total count of all chunks (for statistics)
extern uint32_t total_chunk_count;

// ============================================================
// FUNCTIONS
// ============================================================

int tokenizer_init(void);

// Load text and convert to bytes (initial tokens) - OLD METHOD
void load_text(const char* text, size_t len);

// ============================================================
// NEW: CHUNK-BASED LOADING (the smart way!)
// ============================================================

// Add a unique chunk with its count
// Example: load_chunk(" the", 4, 50000) means " the" appears 50,000 times
void load_chunk(const char* text, size_t len, uint32_t count);

// Print statistics about loaded chunks
void print_chunk_stats(void);

// Count all pairs and find the most frequent one
Pair count_pairs(void);

// Merge a pair into a new token, returns count of occurrences merged
uint32_t merge_pair(Pair pair, uint32_t new_token);

// Run one merge iteration (count -> find max -> merge)
int do_one_merge(void);

// Run multiple merges with checkpoint printing
void do_merges(uint32_t num_merges);

// NEW: Run merges using chunks (the smart way!)
void do_merges_chunks(uint32_t num_merges);

// Get current tokens length
uint32_t get_tokens_len(void);

// Get first N tokens (for display)
void get_first_tokens(uint32_t n, uint32_t* out);

// Get the string representation of a token
const char* get_token_string(uint32_t token);

// Get merge token at index i (merges are in frequency order)
uint32_t get_merge_token(uint32_t i);
