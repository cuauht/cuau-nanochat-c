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

from _tokenizer import lib

lib.tokenizer_init()
