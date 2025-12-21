# Step 1: Environment Setup - Complete Guide

This guide explains every component needed to set up the nanochat development environment, why each dependency matters, and what it does.

## Important: Where to Run Commands

Throughout this guide, you'll see location notes like:
- **📍 Run from: ANYWHERE** - Command can be run from any directory (installs system-wide tools)
- **📍 Run from: PROJECT ROOT** - Command must be run from the nanochat project root directory (where `pyproject.toml` is located)

**What is PROJECT ROOT?** The directory containing `pyproject.toml`, `README.md`, and the `nanochat/` folder. For example: `C:\NANO CHAT`.

## Why Environment Setup Matters

Before writing any code or training any models, you need a properly configured environment. This ensures:
- **Reproducibility**: Same dependencies = same results
- **Isolation**: Your project won't conflict with system Python or other projects
- **Performance**: Fast tokenizers and optimized libraries make training feasible
- **Compatibility**: All components work together correctly

---

## Step 1.1: Install uv (Python Package Manager)

**What it is:**
`uv` is a fast Python package manager written in Rust. It's similar to `pip` but much faster and handles virtual environments automatically.

**Why it matters:**
- **Speed**: Installs packages 10-100x faster than pip
- **Reliability**: Better dependency resolution
- **Simplicity**: One tool handles virtual environments and package installation
- **Lock files**: Ensures exact same versions across machines (`uv.lock`)

**What it does:**
- Creates and manages Python virtual environments
- Installs Python packages from PyPI
- Resolves dependency conflicts
- Generates lock files for reproducible installs

**How to install:**
```powershell
# 📍 Run from: ANYWHERE (installs system-wide)
irm https://astral.sh/uv/install.ps1 | iex

# IMPORTANT: Add uv to PATH immediately after installation
# The installer installs uv to: C:\Users\<YourUsername>\.local\bin
# You MUST add this to PATH or restart PowerShell for uv to work

# Option 1: Temporary fix (only for current PowerShell session)
$env:Path = "$env:USERPROFILE\.local\bin;$env:Path"

# Option 2: PERMANENT fix (recommended - works in all future sessions)
# This adds uv to your User PATH permanently using the registry directly
# IMPORTANT: Expand the path properly and verify it exists
$uvPath = Join-Path $env:USERPROFILE ".local\bin"
$uvPathExpanded = [System.IO.Path]::GetFullPath($uvPath)

# Verify uv.exe actually exists at this location
if (-not (Test-Path "$uvPathExpanded\uv.exe")) {
    Write-Host "WARNING: uv.exe not found at $uvPathExpanded"
    Write-Host "Please verify uv installation completed successfully"
    exit 1
}

# Use registry directly for more reliable PATH modification
$regPath = "Registry::HKEY_CURRENT_USER\Environment"
$currentPath = (Get-ItemProperty -Path $regPath -Name PATH -ErrorAction SilentlyContinue).PATH

# Normalize and check if path already exists
$pathArray = if ($currentPath) { $currentPath -split ';' | Where-Object { $_ -ne '' } } else { @() }
$pathExists = $false
foreach ($path in $pathArray) {
    $normalized = try { [System.IO.Path]::GetFullPath($path) } catch { $path }
    if ($normalized -eq $uvPathExpanded) {
        $pathExists = $true
        break
    }
}

if (-not $pathExists) {
    # Add to User PATH via registry
    $newPath = if ($currentPath) { "$currentPath;$uvPathExpanded" } else { $uvPathExpanded }
    Set-ItemProperty -Path $regPath -Name PATH -Value $newPath -Type ExpandString
    
    Write-Host "✓ Added $uvPathExpanded to User PATH permanently"
    
    # Verify it was written correctly
    $verifyPath = (Get-ItemProperty -Path $regPath -Name PATH).PATH
    if ($verifyPath -like "*$uvPathExpanded*") {
        Write-Host "✓ Verified: PATH was written correctly to registry"
    } else {
        Write-Host "⚠ WARNING: PATH verification failed. Please check manually."
    }
    
    # Also set via .NET method for compatibility
    [System.Environment]::SetEnvironmentVariable("Path", $newPath, [System.EnvironmentVariableTarget]::User)
    
    # Update current session PATH immediately (combine User + Machine PATH)
    $machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    $env:Path = if ($machinePath) { "$newPath;$machinePath" } else { $newPath }
} else {
    Write-Host "✓ $uvPathExpanded is already in User PATH"
    # Still update current session PATH to be safe
    $userPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
    $machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    $env:Path = if ($machinePath) { "$userPath;$machinePath" } else { $userPath }
}
```

**Why this step is needed:**
After installation, `uv` is installed but PowerShell doesn't know where to find it. Without adding it to PATH, you'll get "uv: command not found" error.

**Temporary vs Permanent:**
- **Temporary (`$env:Path`)**: Only works in current PowerShell window. Lost when you close terminal.
- **Permanent (`[Environment]::SetEnvironmentVariable`)**: Works in all future PowerShell/CMD windows. Survives terminal restarts.

**Verify installation:**
```powershell
uv --version
```

**Important Notes:**
- After running the permanent fix, **close ALL PowerShell windows completely** and open a new window
- Windows caches environment variables at the **user session** level (Explorer loads them during sign-in). Closing PowerShell alone is NOT enough if Explorer never reloaded the update.
- Quick ways to force Windows to pick up the new PATH:
  1. Run `RUNDLL32.EXE user32.dll,UpdatePerUserSystemParameters` (fastest), **then** close all terminals and open a new one.
  2. Or restart `explorer.exe` (Task Manager → find Windows Explorer → Restart).
  3. Or sign out/in (or reboot) to guarantee the new PATH is loaded.
- The fix modifies the Windows registry directly for maximum reliability
- PowerShell caches environment variables when it starts, so after forcing Windows to reload the PATH, reopen PowerShell and test again.
- To verify it worked: after the reload, open a brand-new PowerShell window → run `uv --version` (no PATH tweaks should be required)

**If it STILL doesn't work after restarting PowerShell:**
1. Run the diagnostic commands below to check what's happening
2. After running `RUNDLL32.EXE user32.dll,UpdatePerUserSystemParameters`, close ALL PowerShell windows and open a fresh one. If it still fails, restart `explorer.exe` or sign out/in.
3. Try restarting your computer (Windows sometimes caches environment variables aggressively)
4. As a last resort, manually add the path via System Properties (see troubleshooting section)

**Troubleshooting if it still doesn't work:**

Run these diagnostic commands in a NEW PowerShell window (after closing the old one):

```powershell
# 1. Check if uv.exe exists
$uvPath = Join-Path $env:USERPROFILE ".local\bin"
$uvPathExpanded = [System.IO.Path]::GetFullPath($uvPath)
Write-Host "Checking if uv.exe exists at: $uvPathExpanded"
Test-Path "$uvPathExpanded\uv.exe"
# Should return: True

# 2. Check what's actually in User PATH (from registry)
$regPath = "Registry::HKEY_CURRENT_USER\Environment"
$userPath = (Get-ItemProperty -Path $regPath -Name PATH -ErrorAction SilentlyContinue).PATH
Write-Host "`nUser PATH from registry:"
$userPath
# Should contain: C:\Users\<YourUsername>\.local\bin

# 3. Check what PowerShell sees in PATH (current session)
Write-Host "`nCurrent session PATH:"
$env:Path
# Should contain: .local\bin

# 4. Check if PowerShell can find uv
Write-Host "`nTrying to find uv.exe:"
Get-Command uv -ErrorAction SilentlyContinue
# Should show the path to uv.exe

# 5. If PATH contains the path but uv still doesn't work:
# Try using the full path directly
Write-Host "`nTrying full path directly:"
& "$uvPathExpanded\uv.exe" --version
# If this works, the PATH isn't being read correctly by PowerShell
```

**Manual Fix / When Windows Still Ignores the Change:**

If the automatic method still doesn't work, add it manually via Windows GUI:

1. Press `Win + R`, type `sysdm.cpl`, press Enter
2. Click "Advanced" tab → "Environment Variables"
3. Under "User variables" (top section), find "Path" and click "Edit"
4. Click "New" and add: `C:\Users\<YourUsername>\.local\bin` (replace `<YourUsername>` with your actual username)
5. Click OK on all dialogs
6. Run `RUNDLL32.EXE user32.dll,UpdatePerUserSystemParameters` (or restart Explorer / sign out + back in)
7. **Close ALL PowerShell windows** and open a new one
8. Test: `uv --version`

**Common Issues:**
- **PATH is set but PowerShell doesn't see it**: Close ALL PowerShell windows completely, then open a new one. PowerShell reads PATH when it starts.
- **PATH contains wrong format**: The path should be absolute (e.g., `C:\Users\cuauh\.local\bin`), not relative
- **Windows caching**: Sometimes Windows caches environment variables aggressively - try restarting your computer
- **Multiple PowerShell profiles**: If you have multiple PowerShell profiles, make sure you're testing in a clean PowerShell window

**Key commands:**
- `uv venv` - Create virtual environment
- `uv sync` - Install all dependencies from `pyproject.toml`
- `uv run` - Run commands in the virtual environment

---

## Step 1.2: Create Python Virtual Environment

**What it is:**
A virtual environment is an isolated Python environment that contains its own Python interpreter and packages, separate from your system Python.

**Why it matters:**
- **Isolation**: Prevents conflicts with system packages
- **Version control**: Can use different Python versions per project
- **Clean installs**: No interference from other projects
- **Reproducibility**: Same environment on any machine

**What it does:**
- Creates a `.venv/` folder with isolated Python installation
- All packages installed here don't affect system Python
- Can be deleted and recreated easily

**How to create:**
```powershell
# 📍 Run from: PROJECT ROOT (creates .venv folder here)
uv venv
.venv\Scripts\Activate.ps1
```

**Verify activation:** You should see `(.venv)` in your prompt.

---

## Step 1.3: Install Rust and Cargo

**What it is:**
Rust is a systems programming language. Cargo is Rust's package manager and build tool.

**Why it matters:**
- **Custom tokenizer**: nanochat uses a custom Rust BPE tokenizer (`rustbpe`) for speed
- **Performance**: Rust tokenizer is 10-100x faster than pure Python
- **Required dependency**: The project won't build without Rust

**What it does:**
- Provides Rust compiler (`rustc`)
- Cargo manages Rust dependencies and builds Rust projects
- Used to compile `rustbpe` into a Python extension module

**How to install:**
```powershell
# 📍 Run from: ANYWHERE (installs system-wide)
# Option 1: Use winget (recommended)
winget install Rustlang.Rustup

# Option 2: Download installer manually
# Download rustup-init.exe from https://rustup.rs/ and run it
```

**Important:** After installation, add Rust to PATH permanently:
```powershell
# PERMANENT fix (recommended - works in all future sessions)
# Uses registry directly for maximum reliability
$rustPath = Join-Path $env:USERPROFILE ".cargo\bin"
$rustPathExpanded = [System.IO.Path]::GetFullPath($rustPath)

# Verify rustc.exe actually exists at this location
if (-not (Test-Path "$rustPathExpanded\rustc.exe")) {
    Write-Host "WARNING: rustc.exe not found at $rustPathExpanded"
    Write-Host "Please verify Rust installation completed successfully"
    exit 1
}

# Use registry directly for more reliable PATH modification
$regPath = "Registry::HKEY_CURRENT_USER\Environment"
$currentPath = (Get-ItemProperty -Path $regPath -Name PATH -ErrorAction SilentlyContinue).PATH

# Normalize and check if path already exists
$pathArray = if ($currentPath) { $currentPath -split ';' | Where-Object { $_ -ne '' } } else { @() }
$pathExists = $false
foreach ($path in $pathArray) {
    $normalized = try { [System.IO.Path]::GetFullPath($path) } catch { $path }
    if ($normalized -eq $rustPathExpanded) {
        $pathExists = $true
        break
    }
}

if (-not $pathExists) {
    # Add to User PATH via registry
    $newPath = if ($currentPath) { "$currentPath;$rustPathExpanded" } else { $rustPathExpanded }
    Set-ItemProperty -Path $regPath -Name PATH -Value $newPath -Type ExpandString
    
    Write-Host "✓ Added $rustPathExpanded to User PATH permanently"
    
    # Verify it was written correctly
    $verifyPath = (Get-ItemProperty -Path $regPath -Name PATH).PATH
    if ($verifyPath -like "*$rustPathExpanded*") {
        Write-Host "✓ Verified: PATH was written correctly to registry"
    } else {
        Write-Host "⚠ WARNING: PATH verification failed. Please check manually."
    }
    
    # Also set via .NET method for compatibility
    [System.Environment]::SetEnvironmentVariable("Path", $newPath, [System.EnvironmentVariableTarget]::User)
    
    # Update current session PATH immediately (combine User + Machine PATH)
    $machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    $env:Path = if ($machinePath) { "$newPath;$machinePath" } else { $newPath }
} else {
    Write-Host "✓ $rustPathExpanded is already in User PATH"
    # Still update current session PATH to be safe
    $userPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
    $machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    $env:Path = if ($machinePath) { "$userPath;$machinePath" } else { $userPath }
}

**Verify installation:**
```powershell
rustc --version
cargo --version
```

**Important Notes:**
- After running the permanent fix, **close PowerShell completely** and open a new window
- The fix uses `[Environment]::SetEnvironmentVariable` which modifies the Windows registry
- PowerShell caches environment variables, so you MUST restart PowerShell for changes to take effect
- To verify it worked: Close PowerShell → Open new PowerShell → Run `rustc --version` (should work without any PATH commands)

**Key files:**
- `rustbpe/Cargo.toml` - Rust project configuration
- `rustbpe/src/lib.rs` - Rust tokenizer implementation

---

## Step 1.4: Python Dependencies Explained

Now let's understand each Python dependency and why it's needed:

### Core Machine Learning Framework

#### `torch>=2.8.0` (PyTorch)
**What it does:**
- Deep learning framework for building and training neural networks
- Provides tensors, automatic differentiation, GPU acceleration
- Implements the Transformer model architecture

**Why it matters:**
- **Foundation**: Everything runs on PyTorch tensors
- **GPU support**: Enables training on GPUs (CUDA)
- **Autograd**: Automatic gradient computation for backpropagation
- **Distributed training**: Multi-GPU training support

**Used in:**
- Model architecture (`nanochat/gpt.py`)
- Training loops (`scripts/base_train.py`, etc.)
- Data loading and batching

**How to install:**
```powershell
# 📍 Run from: PROJECT ROOT (reads pyproject.toml from here)
# GPU version (CUDA 12.8) - recommended for training
uv sync --extra gpu

# CPU version - for development/testing only
uv sync --extra cpu
```

**Note:** Project supports both CPU and GPU versions:
- `--extra gpu` for CUDA 12.8 support
- `--extra cpu` for CPU-only builds

**Verify installation:**
```powershell
python -c "import torch; print('PyTorch version:', torch.__version__); print('CUDA available:', torch.cuda.is_available())"
```

---

### Tokenization Libraries

#### `tokenizers>=0.22.0` (HuggingFace Tokenizers)
**What it does:**
- Fast tokenization library for training BPE tokenizers
- Provides BPE (Byte Pair Encoding) algorithm implementation
- Handles pre-tokenization, normalization, post-processing

**Why it matters:**
- **Training**: Used to train the initial tokenizer
- **Speed**: Fast Rust-based implementation
- **Flexibility**: Supports GPT-4 style tokenization patterns

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `nanochat/tokenizer.py` - Tokenizer wrapper (HuggingFaceTokenizer class)
- Note: `scripts/tok_train.py` uses `RustBPETokenizer` which wraps HuggingFace tokenizers internally

**Verify installation:**
```powershell
python -c "import tokenizers; print('tokenizers version:', tokenizers.__version__)"
```

#### `tiktoken>=0.11.0` (OpenAI Tiktoken)
**What it does:**
- Fast tokenizer for inference (encoding/decoding text)
- Implements BPE tokenization efficiently
- Used by OpenAI GPT models

**Why it matters:**
- **Inference speed**: Much faster than HuggingFace tokenizers for encoding
- **Compatibility**: Same tokenization as GPT-4 style models
- **Efficiency**: Optimized for production use

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `nanochat/tokenizer.py` - Fast encoding during inference
- Model inference and generation

**Note:** Training uses `tokenizers`, inference uses `tiktoken` for speed.

**Verify installation:**
```powershell
python -c "import tiktoken; print('tiktoken OK')"
```

---

### Data Handling

#### `datasets>=4.0.0` (HuggingFace Datasets)
**What it does:**
- Library for loading and processing datasets
- Handles large datasets efficiently (streaming, caching)
- Provides access to HuggingFace Hub datasets

**Why it matters:**
- **Data loading**: Downloads datasets from HuggingFace Hub
- **Efficiency**: Streaming large datasets without loading everything into memory
- **Standardization**: Common interface for different dataset formats

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `nanochat/dataset.py` - Loading pretraining data
- `tasks/` - Loading evaluation datasets (MMLU, GSM8K, etc.)

**Verify installation:**
```powershell
python -c "import datasets; print('datasets version:', datasets.__version__)"
```

#### `regex>=2025.9.1` (Regular Expressions)
**What it does:**
- Advanced regular expression library
- Supports Unicode, lookahead/lookbehind, etc.

**Why it matters:**
- **Tokenizer pattern**: GPT-4 style regex pattern for text splitting
- **Text preprocessing**: Pattern matching for tokenization
- **Required**: Tokenizer training needs regex patterns

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `nanochat/tokenizer.py` - Used via `tokenizers.Regex` wrapper (not directly imported)
- Text preprocessing before BPE training

**Verify installation:**
```powershell
python -c "import regex; print('regex OK')"
```

---

### Web Framework (for UI)

#### `fastapi>=0.117.1` (FastAPI)
**What it does:**
- Modern web framework for building APIs
- Automatic API documentation
- High performance (async support)

**Why it matters:**
- **Web UI backend**: Powers the ChatGPT-like web interface
- **API endpoints**: Handles chat requests from frontend
- **Easy deployment**: Simple to serve the model

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `scripts/chat_web.py` - Web server for chat interface

**Verify installation:**
```powershell
python -c "import fastapi; print('FastAPI version:', fastapi.__version__)"
```

#### `uvicorn>=0.36.0` (Uvicorn)
**What it does:**
- ASGI server for running FastAPI applications
- Handles HTTP requests and WebSocket connections

**Why it matters:**
- **Server**: Runs the FastAPI web application
- **Performance**: Fast async server implementation
- **Required**: FastAPI needs an ASGI server to run

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `scripts/chat_web.py` - Server for web UI

**Verify installation:**
```powershell
python -c "import uvicorn; print('uvicorn version:', uvicorn.__version__)"
```

---

### Utilities

#### `psutil>=7.1.0` (Process and System Utilities)
**What it does:**
- Cross-platform library for system and process utilities
- CPU, memory, disk usage information

**Why it matters:**
- **Monitoring**: Track resource usage during training
- **System info**: Report system specifications in reports
- **Debugging**: Monitor memory usage to prevent OOM

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Used in:**
- `nanochat/report.py` - System information in reports
- Resource monitoring during training

**Verify installation:**
```powershell
python -c "import psutil; print('psutil version:', psutil.__version__)"
```

#### `setuptools>=80.9.0` (Setuptools)
**What it does:**
- Python packaging and distribution utilities
- Builds Python packages

**Why it matters:**
- **Build system**: Required for building Python packages
- **Package metadata**: Handles package configuration
- **Standard dependency**: Many packages depend on setuptools

**Used in:**
- Package building and installation

#### `files-to-prompt>=0.6` (Files to Prompt)
**What it does:**
- Utility to package codebase files into a single prompt
- Useful for asking LLMs about codebases

**Why it matters:**
- **Documentation**: Can package entire repo for LLM analysis
- **Convenience**: Makes it easy to ask questions about the codebase
- **Optional**: Not critical for training, but useful

**Used in:**
- Packaging codebase for LLM analysis (as mentioned in README)

---

### Experiment Tracking

#### `wandb>=0.21.3` (Weights & Biases)
**What it does:**
- Experiment tracking and visualization platform
- Logs metrics, hyperparameters, system info
- Provides web dashboard for training monitoring

**Why it matters:**
- **Monitoring**: Track training progress in real-time
- **Debugging**: Identify training issues early
- **Comparison**: Compare different training runs
- **Optional**: Can use "dummy" mode to skip logging

**Used in:**
- All training scripts (`scripts/base_train.py`, `scripts/mid_train.py`, etc.)
- Logs loss, learning rate, validation metrics

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**Optional setup (if you want to use wandb):**
```powershell
wandb login
```

**Note:** Set `WANDB_RUN=dummy` to skip wandb logging (default behavior).

**Verify installation:**
```powershell
python -c "import wandb; print('wandb version:', wandb.__version__)"
```

---

### Build System

#### `maturin>=1.7,<2.0` (Maturin)
**What it does:**
- Build tool for Rust-Python extensions
- Compiles Rust code into Python extension modules
- Handles PyO3 bindings (Rust ↔ Python interface)

**Why it matters:**
- **Rust integration**: Builds `rustbpe` Rust code into Python module
- **Required**: Can't use custom tokenizer without maturin
- **Performance**: Enables fast Rust code in Python

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**How to build rustbpe module:**
```powershell
# 📍 Run from: PROJECT ROOT (builds rustbpe module)
uv run maturin develop --release --manifest-path rustbpe/Cargo.toml
```

**Used in:**
- Building `rustbpe` module: `maturin develop --manifest-path rustbpe/Cargo.toml`

**Key files:**
- `pyproject.toml` - Configured to use maturin as build backend
- `rustbpe/Cargo.toml` - Rust project configuration

**Verify installation:**
```powershell
maturin --version
```

---

### Development Dependencies

#### `pytest>=8.0.0` (Pytest)
**What it does:**
- Python testing framework
- Discovers and runs tests

**Why it matters:**
- **Testing**: Run tests to verify code works
- **Quality assurance**: Catch bugs before deployment
- **Development**: Only needed for development, not production

**How to install:**
```powershell
# Installed automatically with: uv sync --extra gpu
```

**How to run tests:**
```powershell
python -m pytest tests/ -v
```

**Used in:**
- `tests/` - Test files for tokenizer and engine

**Verify installation:**
```powershell
pytest --version
```

---

## Step 1.5: Rust Dependencies (for rustbpe)

The `rustbpe` module has its own Rust dependencies:

### `pyo3` (Python ↔ Rust Bindings)
**What it does:**
- Creates Python extension modules from Rust code
- Handles data conversion between Python and Rust

**Why it matters:**
- **Integration**: Makes Rust code callable from Python
- **Required**: Without this, Rust tokenizer can't be used in Python

### `fancy-regex` (Advanced Regex)
**What it does:**
- Advanced regex library for Rust
- Supports lookahead, lookbehind, etc.

**Why it matters:**
- **Tokenizer pattern**: Implements GPT-4 style regex pattern
- **Text splitting**: Pre-processes text before BPE

### `rayon` (Parallel Processing)
**What it does:**
- Data parallelism library for Rust
- Parallel iterators and thread pools

**Why it matters:**
- **Speed**: Parallelizes tokenizer training
- **Performance**: Faster BPE training on multi-core CPUs

### Other Rust Dependencies
- `dary_heap`, `indexmap`, `ahash`, `compact_str`: Data structures for efficient tokenizer implementation
- `log`, `pyo3-log`: Logging utilities

---

## Step 1.6: Base Directory Configuration

**What it is:**
The base directory (`$env:USERPROFILE\.cache\nanochat` or `$NANOCHAT_BASE_DIR`) stores all intermediate artifacts.

**Why it matters:**
- **Organization**: Keeps project directory clean
- **Caching**: Avoids re-downloading data
- **Persistence**: Checkpoints and models survive code changes
- **Standard location**: Uses Windows cache directory convention

**What it stores:**
- `base_data/` - Downloaded pretraining data shards (parquet files)
- `tokenizer/` - Trained tokenizer files
- `base_checkpoints/` - Pretrained model checkpoints
- `mid_checkpoints/` - Midtrained model checkpoints
- `sft_checkpoints/` - SFT model checkpoints
- `identity_conversations.jsonl` - Custom conversation data
- `report/` - Training report sections

**How to set:**
```powershell
# 📍 Run from: ANYWHERE (sets environment variable)
$env:NANOCHAT_BASE_DIR = "$env:USERPROFILE\.cache\nanochat"
New-Item -ItemType Directory -Force -Path $env:NANOCHAT_BASE_DIR
```

**Verify:**
```powershell
echo $env:NANOCHAT_BASE_DIR
```

---

## Complete Setup Commands

Here's the complete setup sequence (copy-paste ready):

```powershell
# 1. Install uv (if not already installed)
# 📍 Run from: ANYWHERE
irm https://astral.sh/uv/install.ps1 | iex

# IMPORTANT: Add uv to PATH PERMANENTLY (works in all future sessions)
# Expand the path properly and verify it exists
$uvPath = Join-Path $env:USERPROFILE ".local\bin"
$uvPathExpanded = [System.IO.Path]::GetFullPath($uvPath)

# Verify uv.exe actually exists
if (-not (Test-Path "$uvPathExpanded\uv.exe")) {
    Write-Host "WARNING: uv.exe not found at $uvPathExpanded - checking installation..."
}

# Get current User PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Check if path is already in PATH (normalize comparison)
$pathArray = $currentPath -split ';' | Where-Object { $_ -ne '' }
$pathExists = $pathArray | Where-Object { 
    $normalized = [System.IO.Path]::GetFullPath($_)
    $normalized -eq $uvPathExpanded
}

if (-not $pathExists) {
    $newPath = if ($currentPath) { "$currentPath;$uvPathExpanded" } else { $uvPathExpanded }
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "✓ Added $uvPathExpanded to User PATH permanently"
    
    # Verify it was written
    $verifyPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($verifyPath -like "*$uvPathExpanded*") {
        Write-Host "✓ Verified: PATH written correctly"
    }
    
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
} else {
    Write-Host "✓ $uvPathExpanded is already in User PATH"
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
}

# 2. Navigate to project directory
cd C:\NANO CHAT

# 3. Create virtual environment
# 📍 Run from: PROJECT ROOT
uv venv
.venv\Scripts\Activate.ps1

# 4. Install Rust/Cargo (if not already installed)
# 📍 Run from: ANYWHERE (can do this before step 2 if you prefer)
winget install Rustlang.Rustup

# After Rust installation, add to PATH PERMANENTLY
# Expand the path properly and verify it exists
$rustPath = Join-Path $env:USERPROFILE ".cargo\bin"
$rustPathExpanded = [System.IO.Path]::GetFullPath($rustPath)

# Verify rustc.exe actually exists
if (-not (Test-Path "$rustPathExpanded\rustc.exe")) {
    Write-Host "WARNING: rustc.exe not found at $rustPathExpanded - checking installation..."
}

# Get current User PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Check if path is already in PATH (normalize comparison)
$pathArray = $currentPath -split ';' | Where-Object { $_ -ne '' }
$pathExists = $pathArray | Where-Object { 
    $normalized = [System.IO.Path]::GetFullPath($_)
    $normalized -eq $rustPathExpanded
}

if (-not $pathExists) {
    $newPath = if ($currentPath) { "$currentPath;$rustPathExpanded" } else { $rustPathExpanded }
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "✓ Added $rustPathExpanded to User PATH permanently"
    
    # Verify it was written
    $verifyPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($verifyPath -like "*$rustPathExpanded*") {
        Write-Host "✓ Verified: PATH written correctly"
    }
    
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
} else {
    Write-Host "✓ $rustPathExpanded is already in User PATH"
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
}

# 5. Install Python dependencies (GPU version)
# 📍 Run from: PROJECT ROOT
uv sync --extra gpu

# 6. Build Rust tokenizer extension
# 📍 Run from: PROJECT ROOT
uv run maturin develop --release --manifest-path rustbpe/Cargo.toml

# 7. Set base directory
# 📍 Run from: ANYWHERE
$env:NANOCHAT_BASE_DIR = "$env:USERPROFILE\.cache\nanochat"
New-Item -ItemType Directory -Force -Path $env:NANOCHAT_BASE_DIR
```

---

## Verification Steps

After setup, verify everything works:

```powershell
# Check Python version (should be >=3.10)
python --version

# Check PyTorch installation
python -c "import torch; print(torch.__version__); print('CUDA available:', torch.cuda.is_available())"

# Check tokenizer libraries
python -c "import tokenizers; import tiktoken; print('Tokenizers OK')"

# Check Rust tokenizer module
python -c "import rustbpe; print('rustbpe OK')"

# Check other key imports
python -c "import datasets, fastapi, wandb; print('All imports OK')"
```

---

## Common Issues and Solutions

### Issue: "uv: command not found"
**Why it happens:** After installing `uv`, PowerShell doesn't know where to find it because the installation directory isn't in PATH yet.

**Temporary Solution (current session only):**
```powershell
# Add uv to PATH for current session only
$env:Path = "$env:USERPROFILE\.local\bin;$env:Path"
uv --version
```

**Permanent Solution (recommended - works forever):**
```powershell
# Add uv to User PATH permanently (works in all future sessions)
# Expand the path properly
$uvPath = Join-Path $env:USERPROFILE ".local\bin"
$uvPathExpanded = [System.IO.Path]::GetFullPath($uvPath)

# Verify uv.exe exists
if (-not (Test-Path "$uvPathExpanded\uv.exe")) {
    Write-Host "ERROR: uv.exe not found at $uvPathExpanded"
    Write-Host "Please reinstall uv first"
    exit 1
}

# Get current User PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Add if not already present (normalize comparison)
$pathArray = $currentPath -split ';' | Where-Object { $_ -ne '' }
$pathExists = $pathArray | Where-Object { 
    $normalized = [System.IO.Path]::GetFullPath($_)
    $normalized -eq $uvPathExpanded
}

if (-not $pathExists) {
    $newPath = if ($currentPath) { "$currentPath;$uvPathExpanded" } else { $uvPathExpanded }
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "✓ Added $uvPathExpanded to User PATH permanently"
    
    # Verify it was written
    $verifyPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($verifyPath -like "*$uvPathExpanded*") {
        Write-Host "✓ Verified: PATH written correctly"
    }
    
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
} else {
    Write-Host "✓ $uvPathExpanded is already in User PATH"
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
}

# Test it works
uv --version
```

**CRITICAL:** After running this, **close PowerShell completely** and open a new window. PowerShell caches environment variables, so the change won't take effect until you restart PowerShell.

### Issue: "rustc: command not found"
**Why it happens:** After installing Rust, PowerShell doesn't know where to find it because the installation directory isn't in PATH yet.

**Temporary Solution (current session only):**
```powershell
# Add Rust to PATH for current session only
$env:Path = "$env:USERPROFILE\.cargo\bin;$env:Path"
rustc --version
cargo --version
```

**Permanent Solution (recommended - works forever):**
```powershell
# Add Rust to User PATH permanently (works in all future sessions)
# Expand the path properly
$rustPath = Join-Path $env:USERPROFILE ".cargo\bin"
$rustPathExpanded = [System.IO.Path]::GetFullPath($rustPath)

# Verify rustc.exe exists
if (-not (Test-Path "$rustPathExpanded\rustc.exe")) {
    Write-Host "ERROR: rustc.exe not found at $rustPathExpanded"
    Write-Host "Please reinstall Rust first"
    exit 1
}

# Get current User PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Add if not already present (normalize comparison)
$pathArray = $currentPath -split ';' | Where-Object { $_ -ne '' }
$pathExists = $pathArray | Where-Object { 
    $normalized = [System.IO.Path]::GetFullPath($_)
    $normalized -eq $rustPathExpanded
}

if (-not $pathExists) {
    $newPath = if ($currentPath) { "$currentPath;$rustPathExpanded" } else { $rustPathExpanded }
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "✓ Added $rustPathExpanded to User PATH permanently"
    
    # Verify it was written
    $verifyPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($verifyPath -like "*$rustPathExpanded*") {
        Write-Host "✓ Verified: PATH written correctly"
    }
    
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
} else {
    Write-Host "✓ $rustPathExpanded is already in User PATH"
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User")
}

# Test it works
rustc --version
cargo --version
```

**CRITICAL:** After running this, **close PowerShell completely** and open a new window. PowerShell caches environment variables, so the change won't take effect until you restart PowerShell.

### Issue: "CUDA not available" in PyTorch
**Solution:**
```powershell
# 📍 Run from: PROJECT ROOT
uv sync --extra gpu
```

### Issue: "maturin build fails"
**Solution:**
```powershell
# Ensure Rust is installed and up to date
rustup update
```

### Issue: "Out of memory during installation"
**Solution:**
```powershell
# 📍 Run from: PROJECT ROOT
uv sync --extra cpu
```

---

## Summary

Environment setup involves:
1. **Package manager** (uv) - Fast, reliable dependency management
2. **Virtual environment** - Isolated Python environment
3. **Rust toolchain** - For custom fast tokenizer
4. **Core ML framework** (PyTorch) - Neural network training
5. **Tokenization libraries** - For text processing
6. **Data libraries** - For dataset handling
7. **Web framework** - For chat UI
8. **Build tools** - For Rust-Python integration
9. **Monitoring** - For experiment tracking
10. **Base directory** - For caching artifacts

Each component is essential for the full pipeline. Missing any piece will cause failures downstream. Take time to verify each step before proceeding to data preparation!

