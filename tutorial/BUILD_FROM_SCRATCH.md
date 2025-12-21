# Building nanochat From Scratch: Step-by-Step Guide

This guide walks you through recreating the nanochat project from scratch, explaining each step and why it matters. The goal is to help you understand the complete pipeline of building a ChatGPT-like LLM.

## Overview

nanochat is a full-stack LLM implementation that includes:
- Tokenization (BPE tokenizer training)
- Pretraining (base language model)
- Midtraining (teaching conversation format)
- Supervised Fine-tuning (SFT)
- Optional Reinforcement Learning (RL)
- Evaluation and inference

The entire pipeline runs end-to-end and produces a chat-capable model.

---

## Step 1: Environment Setup

**What to do:**
- Set up Python virtual environment (using `uv` package manager)
- Install dependencies (PyTorch, tokenizers, datasets, etc.)
- Set up Rust/Cargo (needed for custom BPE tokenizer)
- Configure base directory for caching data/models

**Why it matters:**
- Isolates dependencies from system Python
- Ensures reproducible builds
- Rust tokenizer is faster than pure Python implementations
- Base directory (`~/.cache/nanochat`) stores all intermediate artifacts (data shards, tokenizers, checkpoints)

**Key files:**
- `pyproject.toml` - Python dependencies
- `rustbpe/` - Custom Rust BPE tokenizer implementation

---

## Step 2: Data Preparation (Data Sharding)

**What to do:**
- Download pretraining dataset shards (parquet files)
- Each shard contains ~250M characters (~100MB compressed)
- Shards are stored in `~/.cache/nanochat/base_data/`

**Why it matters:**
- **This is the foundation** - without data, nothing else works
- Sharding allows streaming data during training (don't need entire dataset in memory)
- Parquet format is efficient for large-scale text data
- Shuffled shards ensure diverse training batches
- You need enough shards: calculate as `(target_tokens × chars_per_token) / chars_per_shard`

**Key files:**
- `nanochat/dataset.py` - Downloads and reads parquet shards
- `dev/repackage_data_reference.py` - Reference implementation showing how shards were created

**Example calculation:**
- For d20 model (561M params): need ~11.2B tokens
- At 4.8 chars/token: ~54B characters
- At 250M chars/shard: need ~216 shards (round up to 240)

---

## Step 3: Tokenizer Training

**What to do:**
- Train a BPE (Byte Pair Encoding) tokenizer on ~2B characters of data
- Vocabulary size: 65,536 tokens (2^16)
- Save tokenizer to disk
- Evaluate tokenizer compression ratio

**Why it matters:**
- Tokenizer converts raw text into token IDs that the model understands
- BPE learns subword units (e.g., "ing", "tion") that balance vocabulary size vs. compression
- GPT-4 style regex pattern splits text intelligently before BPE
- Special tokens (`<|bos|>`, `<|user_start|>`, etc.) are added for conversation formatting
- Tokenizer must be trained BEFORE model training (model needs vocab_size)

**Key files:**
- `scripts/tok_train.py` - Trains the tokenizer
- `scripts/tok_eval.py` - Evaluates tokenizer performance
- `nanochat/tokenizer.py` - Tokenizer wrapper and utilities
- `rustbpe/` - Fast Rust implementation for training

**What happens:**
1. Iterate through data shards, yielding text documents
2. Apply GPT-4 style regex splitting
3. Train BPE merges to build vocabulary
4. Save tokenizer encoding (for inference) and token_bytes mapping (for loss evaluation)

---

## Step 4: Base Model Pretraining

**What to do:**
- Initialize GPT Transformer model with random weights
- Train on raw text data (next token prediction)
- Use distributed training across multiple GPUs
- Save checkpoints periodically

**Why it matters:**
- **This is where the model learns language** - pretraining teaches general language understanding
- Transformer architecture (attention mechanism) learns patterns in text
- Training on massive text corpus builds world knowledge and linguistic patterns
- Chinchilla scaling law: train on ~20× more tokens than parameters
- This is the longest and most expensive step (hours/days of GPU time)

**Key files:**
- `scripts/base_train.py` - Main pretraining script
- `nanochat/gpt.py` - Transformer model architecture
- `nanochat/dataloader.py` - Efficient data loading and tokenization
- `nanochat/adamw.py`, `nanochat/muon.py` - Optimizers

**What happens:**
1. Load tokenizer and calculate vocab_size
2. Initialize model architecture (depth, width, heads based on `depth` parameter)
3. Set up distributed data parallel (DDP) for multi-GPU training
4. Create data loader that streams shards, tokenizes on-the-fly
5. Training loop:
   - Forward pass: predict next token
   - Backward pass: compute gradients
   - Optimizer step: update weights (Muon for matrices, AdamW for embeddings)
   - Evaluate validation loss periodically
   - Sample from model to see progress
6. Save final checkpoint

**Key hyperparameters:**
- `depth`: Number of transformer layers (d20 = 20 layers)
- `device_batch_size`: Batch size per GPU
- `total_batch_size`: Effective batch size (after gradient accumulation)
- Learning rates: Different for embeddings vs. matrices

---

## Step 5: Base Model Evaluation

**What to do:**
- Evaluate pretrained model on validation set (bits per byte)
- Run CORE metric evaluation (measures general language understanding)
- Sample text from the model to inspect quality

**Why it matters:**
- Validates that pretraining worked correctly
- CORE metric is a standardized benchmark for base models
- Samples show if model learned basic language patterns
- Establishes baseline before finetuning

**Key files:**
- `scripts/base_loss.py` - Evaluates bits per byte and samples
- `scripts/base_eval.py` - Runs CORE metric evaluation
- `nanochat/core_eval.py` - CORE metric implementation

---

## Step 6: Midtraining

**What to do:**
- Load pretrained base model
- Train on mixture of tasks that teach conversation format:
  - SmolTalk (general conversations)
  - MMLU (multiple choice questions)
  - GSM8K (math problems)
  - Custom identity conversations (optional personality)
  - Spelling tasks
- Use special tokens: `<|user_start|>`, `<|assistant_start|>`, etc.

**Why it matters:**
- **Teaches the model HOW to chat** - base model only predicts next token, doesn't understand conversation structure
- Introduces special tokens that format conversations
- Teaches model to handle multiple choice, math, and structured outputs
- Lower learning rate than pretraining (fine-tuning, not learning from scratch)
- Much shorter than pretraining (hundreds of steps vs. thousands)

**Key files:**
- `scripts/mid_train.py` - Midtraining script
- `tasks/` - Various task implementations (SmolTalk, MMLU, GSM8K, etc.)
- `tasks/common.py` - TaskMixture combines multiple tasks

**What happens:**
1. Load base model checkpoint
2. Create task mixture (samples from multiple datasets)
3. Render conversations into token sequences with special tokens
4. Train with lower learning rate (typically 1.0× base LR)
5. Model learns to respond in conversation format

---

## Step 7: Supervised Fine-Tuning (SFT)

**What to do:**
- Load midtrained model
- Fine-tune on curated conversation datasets
- Focus on specific tasks: ARC, GSM8K, SmolTalk, custom conversations
- Train for 1 epoch (or small number of iterations)

**Why it matters:**
- **Refines conversation quality** - makes model better at specific tasks
- Smaller, higher-quality dataset than midtraining
- Domain adaptation: adapts model to specific use cases
- Very low learning rate (typically 0.02× base LR) - gentle fine-tuning
- Short training time (minutes to hours)

**Key files:**
- `scripts/chat_sft.py` - SFT training script
- `tasks/customjson.py` - Load custom conversation data (e.g., identity conversations)

**What happens:**
1. Load midtrained model
2. Create task mixture with curated datasets
3. Train with very low learning rate
4. Evaluate on validation tasks
5. Model becomes better at following instructions and having conversations

---

## Step 8: Evaluation

**What to do:**
- Evaluate model on benchmark tasks:
  - ARC (science questions)
  - GSM8K (math problems)
  - MMLU (broad knowledge)
  - HumanEval (coding)
  - ChatCORE (conversation quality)
- Generate report card with all metrics

**Why it matters:**
- Measures model performance objectively
- Compares different training stages (base → mid → SFT)
- Identifies strengths and weaknesses
- Report card summarizes entire training run

**Key files:**
- `scripts/chat_eval.py` - Evaluates chat model on tasks
- `nanochat/report.py` - Generates markdown report

---

## Step 9: Optional Reinforcement Learning

**What to do:**
- Train model using RLHF (Reinforcement Learning from Human Feedback)
- Currently implemented for GSM8K (math problems)
- Uses reward model or self-consistency to improve answers

**Why it matters:**
- Can improve performance beyond SFT
- Teaches model to generate better answers (not just mimic training data)
- More advanced technique, optional for basic setup

**Key files:**
- `scripts/chat_rl.py` - RL training script

---

## Step 10: Inference and Web UI

**What to do:**
- Load trained model checkpoint
- Set up inference engine with KV cache
- Deploy web UI (ChatGPT-like interface)
- Chat with your model!

**Why it matters:**
- **This is the end goal** - a working chatbot you can interact with
- KV cache speeds up generation (caches previous token computations)
- Web UI makes it accessible and user-friendly
- Model can now have real conversations

**Key files:**
- `scripts/chat_web.py` - Web UI server
- `scripts/chat_cli.py` - Command-line chat interface
- `nanochat/engine.py` - Efficient inference engine with KV cache
- `nanochat/ui.html` - Frontend HTML/CSS/JS

---

## Summary: The Complete Flow

```
1. Setup Environment
   ↓
2. Download Data Shards (pretraining corpus)
   ↓
3. Train Tokenizer (BPE on ~2B chars)
   ↓
4. Pretrain Base Model (GPT on raw text, ~hours)
   ↓
5. Evaluate Base Model (CORE metric, samples)
   ↓
6. Midtraining (teach conversation format, ~minutes)
   ↓
7. Supervised Fine-Tuning (refine conversations, ~minutes)
   ↓
8. Evaluation (benchmark tasks, report card)
   ↓
9. [Optional] Reinforcement Learning
   ↓
10. Inference & Web UI (chat with your model!)
```

## Key Insights

1. **Data comes first** - Without proper data sharding, nothing works
2. **Tokenizer before model** - Model architecture depends on vocab_size
3. **Pretraining is expensive** - This is where most compute/time is spent
4. **Midtraining teaches format** - Base model doesn't know how to chat
5. **SFT refines quality** - Small improvements with curated data
6. **Each stage builds on previous** - Can't skip steps

## Why This Order Matters

- **Data → Tokenizer → Model**: Each depends on the previous
- **Base → Mid → SFT**: Each stage adds capabilities incrementally
- **Train → Eval → Deploy**: Validate before deploying

## Common Pitfalls

1. **Not enough data shards**: Model will overfit or train too slowly
2. **Wrong vocab size**: Too small = poor compression, too large = wasted parameters
3. **OOM (Out of Memory)**: Reduce `device_batch_size` if GPU runs out of memory
4. **Skipping evaluation**: Won't know if training is working
5. **Wrong learning rates**: Each stage needs different LR schedules

---

## Next Steps

Once you understand this flow, you can:
- Modify model architecture (change depth, width, attention)
- Add new tasks or datasets
- Experiment with hyperparameters
- Customize identity/personality via synthetic data
- Scale up to larger models (more GPUs, more data)

The beauty of nanochat is that it's all in one codebase - you can see and modify every step!

