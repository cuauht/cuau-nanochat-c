# Build: compiles C modules into Python
import os
from pathlib import Path
from cffi import FFI

def build():
    root = Path(__file__).parent.parent
    gen_dir = root / "_generated"
    gen_dir.mkdir(exist_ok=True)
    os.chdir(gen_dir)
    
    ffi = FFI()
    ffi.cdef("""
        typedef struct { uint32_t left; uint32_t right; } Pair;
        int tokenizer_init(void);
        void load_text(const char* text, size_t len);
        Pair count_pairs(void);
        uint32_t merge_pair(Pair pair, uint32_t new_token);
        int do_one_merge(void);
        void do_merges(uint32_t num_merges);
        uint32_t get_tokens_len(void);
        void get_first_tokens(uint32_t n, uint32_t* out);
        const char* get_token_string(uint32_t token);
        uint32_t get_merge_token(uint32_t i);
    """)
    ffi.set_source("_tokenizer", '#include "01_tokenizer.h"',
                   sources=["../c/01_tokenizer.c"],
                   include_dirs=["../c"])
    ffi.compile(verbose=True)

if __name__ == "__main__":
    build()

