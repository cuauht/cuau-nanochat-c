# Building a Tokenizer in C

## Why C? Why not Rust?

Simple: **I already know C.**

I come from embedded systems. I've spent years writing C for microcontrollers, hardware drivers, and real-time systems. When I saw Karpathy's nanochat using Rust, I thought: "Cool, but I don't want to learn Rust just to understand tokenization."

So I'm doing it my way — **C + Python**.

- **C** for the heavy lifting (fast, low-level, I understand it)
- **Python** for coordination (easy, flexible, quick to iterate)

This is how I learn: by rebuilding things in languages I already know.

---

## How It Works

```
Python (coordinator) → calls → C module (tokenizer)
```

We use **cffi** to compile C code into a Python module. No separate executable, no subprocess calls — just clean integration.

```python
from _tokenizer import lib
lib.tokenizer_init()  # This runs C code!
```

---

## What We'll Build

A tokenizer that:
1. Reads text
2. Splits it into tokens
3. Builds a vocabulary
4. Assigns IDs to each token

Nothing fancy. Just the fundamentals.

---

## Let's Go

The C code lives in `c/tokenizer.c`. That's where the magic happens.

Ready? Let's write some C.

---

*Tutorial continues below...*

<!-- TODO: Add step-by-step tokenizer implementation -->

