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

from _tokenizer import lib

print("="*60)
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
print(f"    - Documents in this row group: {len(texts)}")

# Load each document (cap at 10,000 chars)
print(f"\n[4] Loading documents into C tokenizer...")
print("-"*60)

num_loaded = 0
total_bytes = 0
for i, text in enumerate(texts[:10]):  # Start with first 10 docs
    text_str = str(text)[:10000]  # Cap at 10k chars
    text_bytes = text_str.encode('utf-8')
    
    # Show first 50 chars of each document
    preview = text_str[:50].replace('\n', ' ')
    print(f"    Doc {i}: {len(text_bytes):,} bytes | '{preview}...'")
    
    # Show first 10 bytes as token IDs with their characters
    first_bytes = list(text_bytes[:10])
    tokens_str = ", ".join([f"{b}='{chr(b) if 32 <= b <= 126 else '?'}'" for b in first_bytes])
    print(f"           First 10 tokens: {tokens_str}")
    
    lib.load_text(text_bytes, len(text_bytes))
    num_loaded += 1
    total_bytes += len(text_bytes)

print("-"*60)
print(f"\n[5] SUMMARY:")
print(f"    - Documents loaded: {num_loaded}")
print(f"    - Total bytes (tokens): {total_bytes:,}")
print(f"    - Each byte (0-255) is an initial token")
print("="*60)
