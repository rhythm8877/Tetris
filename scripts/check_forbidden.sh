#!/usr/bin/env bash
set -euo pipefail

# Phase 1 charter forbids these in libs/ and game/:
#   - #include <string.h>
#   - #include <math.h>
#   - bare malloc(...)
#   - bare free(...)
#
# fopen/fread/fwrite/fclose are NOT forbidden — they are how libs/fileio.c
# implements persistent leaderboard storage.

PATTERN='#include[[:space:]]*<(string|math)\.h>|\bmalloc\(|\bfree\('

if grep -rEn "$PATTERN" libs/ game/; then
    echo "FORBIDDEN SYMBOL FOUND" >&2
    exit 1
fi
echo "OK — no forbidden includes/calls in libs/ or game/."
