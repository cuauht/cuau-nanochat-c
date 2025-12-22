# Step 1: Tokenizer
#
# ============================================================
# DATA SOURCE
# ============================================================
# 
# Karpathy uses: FineWeb-Edu 100B dataset (educational web text)
# URL: https://huggingface.co/datasets/karpathy/fineweb-edu-100b-shuffle
# Format: Parquet files (shard_00000.parquet, shard_00001.parquet, ...)
# Total shards: 1823 files
#
# For my version:
# - I will download a small subset (1-2 shards) to start
# - Each shard contains text documents in a 'text' column
# - I will iterate through documents and feed them to my C tokenizer
#
# ============================================================
# DATA FLOW
# ============================================================
#
# 1. Download parquet file(s) from HuggingFace
# 2. Read parquet -> extract 'text' column -> get list of documents
# 3. For each document:
#    - Cap at 10,000 chars (avoid super long docs)
#    - Feed to tokenizer
# 4. Stop after processing max_chars (e.g. 10 million chars to start)
#
# ============================================================
# TOKENIZER ALGORITHM (BPE)
# ============================================================
#
# 1. Convert all text to bytes (each byte 0-255 = initial token)
# 2. Count all adjacent pairs in the data
# 3. Find the most frequent pair
# 4. Merge that pair into a new token (256, 257, 258, ...)
# 5. Repeat steps 2-4 until vocab_size reached
# 6. Save vocabulary (token -> bytes mapping)
#
# Example:
#   "aaab" -> [97, 97, 97, 98]  (ascii bytes)
#   Most frequent pair: (97, 97) appears 2 times
#   Merge (97, 97) -> new token 256
#   Result: [256, 97, 98]
#   Keep merging until vocab_size (e.g. 65536)
#
# ============================================================
# OUTPUT
# ============================================================
#
# - vocabulary file: maps token_id -> bytes
# - merges file: list of (pair, new_token) in order they were merged
#
# ============================================================

import sys
sys.path.insert(0, "_generated")
import pyarrow.parquet as pq
import argparse
import regex  # pip install regex (supports Unicode patterns)
from collections import Counter  # For counting unique chunks!

from _tokenizer import lib, ffi

# GPT-4 style pattern - splits text into words, numbers, punctuation
# This prevents tokens from crossing word boundaries!
SPLIT_PATTERN = regex.compile(
    r"""'(?i:[sdmt]|ll|ve|re)|[^\r\n\p{L}\p{N}]?+\p{L}+|\p{N}{1,2}| ?[^\s\p{L}\p{N}]++[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+"""
)

# Parse command line arguments FIRST - so we know what we're running
parser = argparse.ArgumentParser(description='Train BPE tokenizer')
parser.add_argument('--vocabsize', type=int, default=10000, help='Target vocabulary size (default: 10000)')
parser.add_argument('--docs', type=int, default=100, help='Number of documents to process (default: 100)')
args = parser.parse_args()

target_vocab = args.vocabsize
num_docs = args.docs
num_merges = target_vocab - 256  # Subtract base 256 tokens

# Print configuration at the very top
print("="*60)
print(f"TOKENIZER CONFIGURATION:")
print(f"  Target vocabulary size: {target_vocab:,}")
print(f"  Number of documents: {num_docs:,}")
print(f"  Number of merges to perform: {num_merges:,}")
print("="*60)

if num_merges <= 0:
    print("Error: vocab_size must be > 256")
    sys.exit(1)

print("\n" + "="*60)
print("STEP 1: TOKENIZER - LOADING DATA")
print("="*60)

# Initialize tokenizer
print("\n[1] Initializing tokenizer...")
lib.tokenizer_init()

# Load dataset
parquet_file = "data/shard_00000.parquet"
print(f"\n[2] Opening parquet file: {parquet_file}")
pf = pq.ParquetFile(parquet_file)
print(f"    - Number of row groups: {pf.num_row_groups}")

# Read first row group and extract text
print(f"\n[3] Reading row group 0...")
rg = pf.read_row_group(0)
texts = rg.column('text').to_pylist()
total_docs_available = len(texts) * pf.num_row_groups  # Approximate total
print(f"    - Documents in this row group: {len(texts)}")
print(f"    - Total documents available (approx): {total_docs_available:,} ({pf.num_row_groups} row groups × {len(texts)} docs)")
print(f"    - We will process: {min(num_docs, len(texts)):,} documents")

# ============================================================
# NEW SMART APPROACH: Count unique chunks, load each once!
# ============================================================
#
# OLD WAY: Load " the" 50,000 times → slow!
# NEW WAY: Count " the" appears 50,000 times, load once with count
#

print(f"\n[4] Pre-tokenizing documents (splitting into chunks)...")
print("-"*60)

# Step 4a: Split all documents into chunks and COUNT them
chunk_counter = Counter()  # This counts how many times each chunk appears
num_loaded = 0
total_raw_chunks = 0

for i, text in enumerate(texts[:num_docs]):  # Process num_docs documents
    text_str = str(text)[:10000]  # Cap at 10k chars
    
    # PRE-TOKENIZE: Split into chunks using GPT-4 pattern
    chunks = SPLIT_PATTERN.findall(text_str)
    
    # Count each chunk (this is the magic!)
    # If " the" appears 100 times, Counter remembers: {" the": 100}
    chunk_counter.update(chunks)
    
    # Show progress (only first 5, last 2, and every 100th)
    if i < 5 or i >= num_docs - 2 or (i + 1) % 100 == 0:
        preview = text_str[:50].replace('\n', ' ')
        print(f"    Doc {i}: {len(chunks):,} chunks | '{preview}...'")
    elif i == 5:
        print(f"    ... (processing {num_docs - 7} more documents) ...")
    
    total_raw_chunks += len(chunks)
    num_loaded += 1

print("-"*60)
print(f"\n[5] CHUNK COUNTING RESULTS:")
print(f"    - Documents processed: {num_loaded}")
print(f"    - Total chunks found: {total_raw_chunks:,}")
print(f"    - UNIQUE chunks: {len(chunk_counter):,}")
print(f"    - Duplicates avoided: {total_raw_chunks - len(chunk_counter):,}")
print(f"")
print(f"    This means we scan {len(chunk_counter):,} chunks instead of {total_raw_chunks:,}!")
print(f"    That's {total_raw_chunks / max(1, len(chunk_counter)):.1f}x less work!")

# Show top 10 most common chunks
print(f"\n    TOP 10 MOST COMMON CHUNKS:")
print(f"    " + "-"*50)
for chunk, count in chunk_counter.most_common(10):
    # Make it printable
    display = chunk.replace('\n', '\\n')
    if len(display) > 20:
        display = display[:20] + "..."
    print(f"      '{display}' appears {count:,} times")

# Step 4b: Load each UNIQUE chunk into C with its count
print(f"\n[6] Loading {len(chunk_counter):,} unique chunks into C...")
print("-"*60)

total_bytes = 0
for chunk, count in chunk_counter.items():
    chunk_bytes = chunk.encode('utf-8')
    lib.load_chunk(chunk_bytes, len(chunk_bytes), count)
    total_bytes += len(chunk_bytes)

# Show chunk statistics from C
lib.print_chunk_stats()

print(f"\n[7] LOADING COMPLETE:")
print(f"    - Unique chunks loaded: {len(chunk_counter):,}")
print(f"    - Total bytes: {total_bytes:,}")
print(f"    - Each chunk stored ONCE with its count (smart!)")
print("="*60)

# Now do BPE merges
print("\n" + "="*60)
print("STEP 2: BPE MERGING (using SMART chunk method!)")
print("="*60)
print(f"\n[8] Performing {num_merges} merges (target vocab: {target_vocab})...")
print("-"*60)

# Do all merges using the NEW chunk-based method!
lib.do_merges_chunks(num_merges)

print("\n" + "="*60)
print("FINAL STATS:")
print("="*60)
print(f"    - Total merges: {num_merges}")
print(f"    - Final vocab size: {target_vocab}")
print(f"    - Final tokens length: {lib.get_tokens_len():,}")

# Show first 100 merged tokens (these are in FREQUENCY ORDER - most common pairs first)
print(f"\n    First 100 MERGES (in order of frequency):")
print(f"    (Token 256 = most common pair, 257 = second most common, etc.)")
for i in range(min(100, num_merges)):
    token = lib.get_merge_token(i)
    token_str = ffi.string(lib.get_token_string(token)).decode('utf-8', errors='replace')
    # Show printable version (replace non-printable chars with markers)
    display_str = ''.join(c if 32 <= ord(c) <= 126 else '_' for c in token_str[:20])
    print(f"      Merge {i+1}: token{token}='{display_str}'")

print("="*60)
