# Coordinator: runs all training steps
import subprocess
import sys
import os
from pathlib import Path

ROOT = Path(__file__).parent
os.chdir(ROOT)

# Auto-build if needed
if not (ROOT / "_generated").exists():
    print("Building C modules...")
    from scripts.build import build
    build()
    os.chdir(ROOT)

def run(file):
    return subprocess.run([sys.executable, file]).returncode == 0

for step in [1, 2, 3]:
    match step:
        case 1: ok = run("01_tokenizer.py")
        case 2: ok = True  # run("02_dataprep.py")
        case 3: ok = True  # run("03_training.py")
    
    if not ok:
        print(f"Failed at step {step}")
        sys.exit(1)

print("\n=== Done ===")
