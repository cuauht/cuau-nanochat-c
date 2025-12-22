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

// Table of token strings (what each token represents)
char** token_strings = NULL;
uint32_t token_strings_capacity = 0;

// ============================================================
// NEW: CHUNK STORAGE (the smart way!)
// ============================================================

// Array of unique chunks - each chunk stored ONCE with a count
Chunk* chunks = NULL;

// How many unique chunks we have
uint32_t num_chunks = 0;

// Capacity of chunks array
uint32_t chunks_capacity = 0;

// Total count of all chunks (sum of all chunk counts)
uint32_t total_chunk_count = 0;

// ============================================================
// FUNCTIONS
// ============================================================

int tokenizer_init(void) {
    printf("    C: Tokenizer initialized\n");
    printf("    C: tokens[] array ready (starts empty)\n");
    
    // Initialize token_strings table with base 256 tokens
    token_strings_capacity = 65536;  // Max vocab size
    token_strings = calloc(token_strings_capacity, sizeof(char*));
    
    // Fill in tokens 0-255 with their single character
    for (int i = 0; i < 256; i++) {
        token_strings[i] = malloc(2);
        token_strings[i][0] = (char)i;
        token_strings[i][1] = '\0';
    }
    printf("    C: token_strings[] initialized (0-255 = single bytes)\n");
    
    return 0;
}

// Load text and convert to bytes (initial tokens) - OLD METHOD
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

// ============================================================
// NEW: CHUNK-BASED LOADING (the smart way!)
// ============================================================
//
// Instead of loading " the" 50,000 times into one big array,
// we store it ONCE with count=50000.
//
// This is like saying:
//   "I saw ' the' 50,000 times in the text"
// Instead of:
//   "Here's ' the', and ' the', and ' the'..." (50,000 times)
//

void load_chunk(const char* text, size_t len, uint32_t count) {
    // ========================================
    // STEP 1: Make room if needed
    // ========================================
    if (num_chunks >= chunks_capacity) {
        // Grow the array (start with 1000, double each time)
        chunks_capacity = (chunks_capacity == 0) ? 1000 : chunks_capacity * 2;
        chunks = realloc(chunks, chunks_capacity * sizeof(Chunk));
    }
    
    // ========================================
    // STEP 2: Create the new chunk
    // ========================================
    Chunk* c = &chunks[num_chunks];
    
    // Allocate space for this chunk's tokens
    c->tokens = malloc(len * sizeof(uint32_t));
    c->len = (uint32_t)len;
    c->count = count;
    
    // Convert each byte to a token (0-255)
    for (size_t i = 0; i < len; i++) {
        c->tokens[i] = (uint8_t)text[i];
    }
    
    // ========================================
    // STEP 3: Update counters
    // ========================================
    num_chunks++;
    total_chunk_count += count;
}

// Helper: Show a chunk's content in a readable way
static void print_chunk_preview(Chunk* c, int max_chars) {
    printf("'");
    for (uint32_t i = 0; i < c->len && i < (uint32_t)max_chars; i++) {
        char ch = (char)c->tokens[i];
        // Show printable chars, replace others with _
        if (ch >= 32 && ch <= 126) {
            printf("%c", ch);
        } else if (ch == '\n') {
            printf("\\n");
        } else {
            printf("_");
        }
    }
    if (c->len > (uint32_t)max_chars) {
        printf("...");
    }
    printf("'");
}

// Print statistics about loaded chunks
void print_chunk_stats(void) {
    printf("\n");
    printf("    ============================================================\n");
    printf("    C: CHUNK STORAGE COMPLETE\n");
    printf("    ============================================================\n");
    printf("\n");
    printf("    Unique chunks stored: %u\n", num_chunks);
    printf("    Total occurrences:    %u\n", total_chunk_count);
    printf("\n");
    
    // Show a few example chunks
    printf("    SAMPLE CHUNKS (first 5):\n");
    printf("    ------------------------------------------------------------\n");
    for (uint32_t i = 0; i < 5 && i < num_chunks; i++) {
        printf("      [%u] count=%u, len=%u, ", i, chunks[i].count, chunks[i].len);
        print_chunk_preview(&chunks[i], 25);
        printf("\n");
    }
    
    if (num_chunks > 5) {
        printf("      ... and %u more chunks ...\n", num_chunks - 5);
    }
    
    printf("    ============================================================\n");
}

// Simple hash function for pairs
static uint32_t pair_hash(Pair p) {
    return (p.left * 256 + p.right) % 65536;
}

// Helper: format token for display - uses token_strings lookup table
static void format_token(uint32_t token, char* buf, size_t buf_size) {
    // Get the string from our lookup table
    const char* str = token_strings[token];
    size_t j = 0;
    
    buf[j++] = '\'';
    for (size_t i = 0; str[i] && j < buf_size - 2; i++) {
        char c = str[i];
        // Show printable chars, replace others with _
        buf[j++] = (c >= 32 && c <= 126) ? c : '_';
    }
    buf[j++] = '\'';
    buf[j] = '\0';
}

// ============================================================
// OLD: Count pairs from tokens[] array (kept for reference)
// ============================================================
Pair count_pairs(void) {
    uint32_t* counts = calloc(65536, sizeof(uint32_t));
    Pair* pairs = calloc(65536, sizeof(Pair));
    
    for (uint32_t i = 0; i < tokens_len - 1; i++) {
        if (tokens[i] == 0 || tokens[i+1] == 0) continue;
        
        Pair p = {tokens[i], tokens[i+1]};
        uint32_t idx = pair_hash(p);
        
        while (counts[idx] > 0 && (pairs[idx].left != p.left || pairs[idx].right != p.right)) {
            idx = (idx + 1) % 65536;
        }
        
        if (counts[idx] == 0) {
            pairs[idx] = p;
        }
        counts[idx]++;
    }
    
    uint32_t max_count = 0;
    Pair max_pair = {0, 0};
    for (uint32_t i = 0; i < 65536; i++) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            max_pair = pairs[i];
        }
    }
    
    free(counts);
    free(pairs);
    
    return max_pair;
}

// ============================================================
// NEW: Count pairs from chunks[] array (the smart way!)
// ============================================================
//
// Instead of scanning 50,000 occurrences of " the", we:
// 1. Scan " the" ONCE
// 2. Multiply the pair counts by 50,000
//
// This is MUCH faster!
//
static uint32_t last_max_count = 0;  // Store for display

Pair count_pairs_chunks(void) {
    // Hash table for pair counts (bigger for more tokens)
    uint32_t table_size = 1 << 20;  // 1 million entries
    uint32_t* counts = calloc(table_size, sizeof(uint32_t));
    Pair* pairs = calloc(table_size, sizeof(Pair));
    
    // ========================================
    // STEP 1: Scan each unique chunk ONCE
    // ========================================
    for (uint32_t c = 0; c < num_chunks; c++) {
        Chunk* chunk = &chunks[c];
        
        // Skip tiny chunks (no pairs possible)
        if (chunk->len < 2) continue;
        
        // Count pairs within this chunk
        for (uint32_t i = 0; i < chunk->len - 1; i++) {
            Pair p = {chunk->tokens[i], chunk->tokens[i+1]};
            
            // Hash with linear probing
            uint32_t idx = ((p.left * 31337) + p.right) % table_size;
            while (counts[idx] > 0 && (pairs[idx].left != p.left || pairs[idx].right != p.right)) {
                idx = (idx + 1) % table_size;
            }
            
            if (counts[idx] == 0) {
                pairs[idx] = p;
            }
            
            // THE MAGIC: Multiply by chunk count!
            // If " the" appears 50,000 times, this pair gets +50,000
            counts[idx] += chunk->count;
        }
    }
    
    // ========================================
    // STEP 2: Find the most frequent pair
    // ========================================
    uint32_t max_count = 0;
    Pair max_pair = {0, 0};
    
    for (uint32_t i = 0; i < table_size; i++) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            max_pair = pairs[i];
        }
    }
    
    last_max_count = max_count;  // Save for display
    
    free(counts);
    free(pairs);
    
    return max_pair;
}

// ============================================================
// NEW: Merge a pair within all chunks
// ============================================================
//
// When we merge (t, h) -> "th", we need to update every chunk
// that contains this pair.
//
uint32_t merge_pair_chunks(Pair pair, uint32_t new_token) {
    uint32_t total_merged = 0;
    
    // Go through each chunk and apply the merge
    for (uint32_t c = 0; c < num_chunks; c++) {
        Chunk* chunk = &chunks[c];
        
        // Skip tiny chunks
        if (chunk->len < 2) continue;
        
        // Create new token array for this chunk
        uint32_t* new_tokens = malloc(chunk->len * sizeof(uint32_t));
        uint32_t new_len = 0;
        uint32_t merged_in_chunk = 0;
        
        // Scan and merge
        for (uint32_t i = 0; i < chunk->len; i++) {
            if (i < chunk->len - 1 && 
                chunk->tokens[i] == pair.left && 
                chunk->tokens[i+1] == pair.right) {
                // Found the pair! Replace with new token
                new_tokens[new_len++] = new_token;
                i++;  // Skip next token (we merged it)
                merged_in_chunk++;
            } else {
                new_tokens[new_len++] = chunk->tokens[i];
            }
        }
        
        // If we merged anything, update the chunk
        if (merged_in_chunk > 0) {
            free(chunk->tokens);
            chunk->tokens = new_tokens;
            chunk->len = new_len;
            // Total merged = occurrences * chunk count
            total_merged += merged_in_chunk * chunk->count;
        } else {
            free(new_tokens);
        }
    }
    
    // Record this merge in our merge list
    merges = realloc(merges, (merges_len + 1) * sizeof(Merge));
    merges[merges_len].pair = pair;
    merges[merges_len].new_token = new_token;
    merges_len++;
    
    // Store the string representation of this new token
    char* left_s = token_strings[pair.left];
    char* right_s = token_strings[pair.right];
    size_t str_len = strlen(left_s) + strlen(right_s) + 1;
    token_strings[new_token] = malloc(str_len);
    strcpy(token_strings[new_token], left_s);
    strcat(token_strings[new_token], right_s);
    
    return total_merged;
}

// ============================================================
// NEW: Do merges using chunks (the smart way!)
// ============================================================
void do_merges_chunks(uint32_t num_merges_to_do) {
    printf("\n    C: Starting %u merges (using SMART chunk method)...\n", num_merges_to_do);
    printf("    C: We have %u unique chunks to process\n", num_chunks);
    printf("\n");
    
    for (uint32_t i = 0; i < num_merges_to_do; i++) {
        // Count pairs and find most frequent
        Pair max_pair = count_pairs_chunks();
        
        // If no pairs found, we're done
        if (max_pair.left == 0 && max_pair.right == 0 && last_max_count == 0) {
            printf("    C: No more pairs to merge (stopped at merge %u)\n", i);
            break;
        }
        
        // Create new token ID
        uint32_t new_token = 256 + merges_len;
        
        // Merge the pair in all chunks
        uint32_t count = merge_pair_chunks(max_pair, new_token);
        
        // Print progress at checkpoints
        int verbose = (i == 0 || i == num_merges_to_do - 1 || (i + 1) % 100 == 0);
        
        if (verbose) {
            char left_str[32], right_str[32], new_str[32];
            format_token(max_pair.left, left_str, sizeof(left_str));
            format_token(max_pair.right, right_str, sizeof(right_str));
            format_token(new_token, new_str, sizeof(new_str));
            printf("    Merge %u/%u: %s + %s -> %s (count: %u)\n", 
                   i + 1, num_merges_to_do, left_str, right_str, new_str, count);
        }
    }
    
    printf("\n    C: Completed %u merges. Final vocab size: %u\n", 
           num_merges_to_do, 256 + merges_len);
}

// Merge a pair into a new token (replace all occurrences)
// Returns: number of occurrences merged
uint32_t merge_pair(Pair pair, uint32_t new_token) {
    // Create new array for merged tokens
    uint32_t* new_tokens = malloc(tokens_len * sizeof(uint32_t));
    uint32_t new_len = 0;
    uint32_t count = 0;
    
    // Scan tokens and merge pairs
    for (uint32_t i = 0; i < tokens_len; i++) {
        if (i < tokens_len - 1 && tokens[i] == pair.left && tokens[i+1] == pair.right) {
            new_tokens[new_len++] = new_token;
            i++; // Skip next token (we merged it)
            count++;
        } else {
            new_tokens[new_len++] = tokens[i];
        }
    }
    
    // Replace old tokens with new ones
    free(tokens);
    tokens = new_tokens;
    tokens_len = new_len;
    
    // Record this merge
    merges = realloc(merges, (merges_len + 1) * sizeof(Merge));
    merges[merges_len].pair = pair;
    merges[merges_len].new_token = new_token;
    merges_len++;
    
    // Store the string representation of this new token
    // new_token_string = token_strings[left] + token_strings[right]
    char* left_s = token_strings[pair.left];
    char* right_s = token_strings[pair.right];
    size_t str_len = strlen(left_s) + strlen(right_s) + 1;
    token_strings[new_token] = malloc(str_len);
    strcpy(token_strings[new_token], left_s);
    strcat(token_strings[new_token], right_s);
    
    return count;
}

// Run one merge iteration: count pairs -> find max -> merge
int do_one_merge(void) {
    if (tokens_len < 2) {
        printf("    C: Not enough tokens to merge\n");
        return 0;
    }
    
    // Count pairs and find most frequent
    Pair max_pair = count_pairs();
    uint32_t new_token = 256 + merges_len;
    
    // Merge and print result
    uint32_t count = merge_pair(max_pair, new_token);
    char left_str[32], right_str[32], new_str[32];
    format_token(max_pair.left, left_str, sizeof(left_str));
    format_token(max_pair.right, right_str, sizeof(right_str));
    format_token(new_token, new_str, sizeof(new_str));
    printf("    Found %u x %s + %s -> %s\n", count, left_str, right_str, new_str);
    
    return 1;
}

// Run multiple merges with checkpoint printing
void do_merges(uint32_t num_merges) {
    printf("\n    C: Starting %u merges...\n", num_merges);
    
    for (uint32_t i = 0; i < num_merges; i++) {
        if (tokens_len < 2) {
            printf("    C: Not enough tokens to merge (stopped at merge %u)\n", i);
            return;
        }
        
        // Count pairs and find most frequent
        Pair max_pair = count_pairs();
        uint32_t new_token = 256 + merges_len;
        
        // Merge the pair
        uint32_t count = merge_pair(max_pair, new_token);
        
        // Print checkpoint every 100 merges or on first/last merge
        int verbose = (i == 0 || i == num_merges - 1 || (i + 1) % 100 == 0);
        
        if (verbose) {
            char left_str[32], right_str[32], new_str[32];
            format_token(max_pair.left, left_str, sizeof(left_str));
            format_token(max_pair.right, right_str, sizeof(right_str));
            format_token(new_token, new_str, sizeof(new_str));
            printf("    Merge %u/%u: found %u x %s + %s -> %s\n", 
                   i + 1, num_merges, count, left_str, right_str, new_str);
            printf("    ------------------------------------------------------------\n");
        }
    }
    
    printf("    C: Completed %u merges. Final vocab size: %u\n", num_merges, 256 + merges_len);
}

// Get current tokens length
uint32_t get_tokens_len(void) {
    return tokens_len;
}

// Get first N tokens (for display)
void get_first_tokens(uint32_t n, uint32_t* out) {
    uint32_t count = (n < tokens_len) ? n : tokens_len;
    for (uint32_t i = 0; i < count; i++) {
        out[i] = tokens[i];
    }
}

// Get the string representation of a token
const char* get_token_string(uint32_t token) {
    if (token < token_strings_capacity && token_strings[token] != NULL) {
        return token_strings[token];
    }
    return "?";
}

// Get merge info: returns the new_token for merge at index i
uint32_t get_merge_token(uint32_t i) {
    if (i < merges_len) {
        return merges[i].new_token;
    }
    return 0;
}
