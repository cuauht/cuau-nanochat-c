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
        int tokenizer_init(void);
        void load_text(const char* text, size_t len);
    """)
    ffi.set_source("_tokenizer", '#include "01_tokenizer.h"',
                   sources=["../c/01_tokenizer.c"],
                   include_dirs=["../c"])
    ffi.compile(verbose=True)

if __name__ == "__main__":
    build()

