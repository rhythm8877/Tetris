# Tetris (CS Capstone, Phase 2)

A from-scratch Tetris implementation written in C. Built as an
Operating Systems capstone — no `<string.h>`, no `<math.h>`, no
`malloc`/`free`.

## Build

    make

## Run

    ./tetris

## Controls

- Arrow keys — move
- Up — rotate
- Space — hard drop
- C — hold
- P — pause
- Q — quit

## Architecture

- `libs/` — five custom OS-level libraries (math, string, memory,
  screen, keyboard) plus four Phase 2 additions (safe, rng, timer,
  fileio).
- `game/` — game-specific code (piece, board, score, menu, main).
- `data/` — persistent leaderboard (`scores.dat`).

## Rubric mapping

See `plan.md`.

## Verification

    scripts/check_forbidden.sh

Should exit 0 with no output beyond the OK line.

## Authors

- Amod
- Rhythm
- Sai Kiran
