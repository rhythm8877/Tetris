# 🕹️ Tetris OS-Capstone

![Tetris CLI Preview](tetris_cli_preview_1781284554801.png)

## 🚀 The Challenge: Zero Dependencies
Designing a Tetris game is a standard programming exercise. Building one **without the C Standard Library (libc)** is a deep-dive into systems engineering.

This project was built from the ground up for an Operating Systems capstone. It operates without:
- `<string.h>`
- `<math.h>`
- `<stdlib.h>` (No `malloc`, `free`, or `rand`)
- Any standard I/O except raw syscalls

## 🛠️ Custom OS-Level Libraries
To bypass `libc`, we implemented a standalone software stack:
- **`libs/math.c`**: custom integer arithmetic including **Russian Peasant Multiplication** and **Binary Long Division** ($O(\log n)$ performance).
- **`libs/keyboard.c`**: A terminal driver using raw POSIX mode and non-blocking I/O for sub-millisecond input response.
- **`libs/rng.c`**: **Xorshift32** PRNG implementation with a "Fair-Play" **7-Bag Randomizer** used in modern Tetris championships.
- **`libs/screen.c`**: ANSI escape-sequence based rendering engine for flickering-free 60 FPS output.
- **`libs/score.c`**: Persistent leaderboard system with **CRC-based integrity validation** and in-memory sorting.

## 🏗️ Architecture
- **`game/`**: High-level logic (piece rotation, collision detection, game loop).
- **`libs/`**: Low-level "OS" abstractions.
- **`data/`**: Persistent storage (`scores.dat`).

## 📥 Getting Started

### Build
Compile the project using the optimized static makefile:
```bash
make
```

### Run
```bash
./tetris
```

## 🎮 Controls
- **Arrow Keys**: Move & Rotate (Up to rotate)
- **Space**: Hard Drop
- **C**: Hold Piece
- **P**: Pause
- **Q**: Quit

## ✅ Verification
Ensure no forbidden headers were sneaked in via the audit script:
```bash
scripts/check_forbidden.sh
```

## 👥 Authors
- **Amod**
- **Rhythm Jain**
- **Sai Kiran**
