/*
 * game/board.c
 * Author : Amod
 * Desc   : Playfield state — collision, locking, line clears, scoring.
 * Date   : {{DATE}}
 */

#include <stdio.h>

#include "board.h"
#include "piece.h"
#include "../libs/math.h"
#include "../libs/memory.h"
#include "../libs/safe.h"

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static int cell_index(int r, int c)
{
    return my_mul(r, BOARD_W) + c;
}

/* ------------------------------------------------------------------ */
/*  Allocation                                                         */
/* ------------------------------------------------------------------ */

Board *board_new(void)
{
    Board *b;
    int    total;
    int    i;

    b = (Board *)my_alloc((int)sizeof(Board));
    safe_assert(b != (void *)0);

    total    = my_mul(BOARD_W, BOARD_H);
    b->cells = (int *)my_alloc(my_mul(total, (int)sizeof(int)));
    safe_assert(b->cells != (void *)0);

    /* zero-init cells */
    for (i = 0; i < total; i++)
        b->cells[i] = 0;

    b->score = 0;
    b->level = 0;
    b->lines = 0;
    return b;
}

void board_free(Board *b)
{
    if (b) {
        if (b->cells)
            my_dealloc(b->cells);
        my_dealloc(b);
    }
}

/* ------------------------------------------------------------------ */
/*  Collision                                                          */
/* ------------------------------------------------------------------ */

int board_at(const Board *b, int r, int c)
{
    /* Above the top → empty (piece spawning above the skyline) */
    if (r < 0)
        return 0;
    /* Floor or walls → solid */
    if (r >= BOARD_H || c < 0 || c >= BOARD_W)
        return 1;
    return b->cells[cell_index(r, c)] != 0;
}

int board_can_place(const Board *b, const Piece *p,
                    int drow, int dcol, int drot)
{
    int dr, dc;
    int rot;

    rot = my_mod(p->rot + drot + 4, 4);

    for (dr = 0; dr < PIECE_BOX; dr++) {
        for (dc = 0; dc < PIECE_BOX; dc++) {
            if (piece_cell(p->type, rot, dr, dc)) {
                int r = p->row + drow + dr;
                int c = p->col + dcol + dc;
                if (board_at(b, r, c))
                    return 0;
            }
        }
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Locking                                                            */
/* ------------------------------------------------------------------ */

void board_lock(Board *b, const Piece *p)
{
    int dr, dc;

    for (dr = 0; dr < PIECE_BOX; dr++) {
        for (dc = 0; dc < PIECE_BOX; dc++) {
            if (piece_cell(p->type, p->rot, dr, dc)) {
                int r = p->row + dr;
                int c = p->col + dc;
                safe_assert(r >= 0);
                b->cells[cell_index(r, c)] = p->color_id;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Line clearing                                                      */
/* ------------------------------------------------------------------ */

int board_clear_lines(Board *b)
{
    int r, c, full, src, dst, cleared;

    cleared = 0;
    r = BOARD_H - 1;

    while (r >= 0) {
        /* Check if row r is full */
        full = 1;
        for (c = 0; c < BOARD_W; c++) {
            if (b->cells[cell_index(r, c)] == 0) {
                full = 0;
                break;
            }
        }

        if (!full) {
            r--;
            continue;
        }

        /* Row r is full — shift everything above it down by one */
        for (src = r - 1; src >= 0; src--) {
            dst = src + 1;
            for (c = 0; c < BOARD_W; c++)
                b->cells[cell_index(dst, c)] = b->cells[cell_index(src, c)];
        }

        /* Clear the topmost row */
        for (c = 0; c < BOARD_W; c++)
            b->cells[cell_index(0, c)] = 0;

        cleared++;
        /* Do NOT decrement r — re-check same index */
    }

    return cleared;
}

/* ------------------------------------------------------------------ */
/*  Scoring                                                            */
/* ------------------------------------------------------------------ */

void board_apply_score(Board *b, int lines_cleared)
{
    static const int TABLE[5] = { 0, 100, 300, 500, 800 };
    int idx;

    /* Note: this updates score only. The lines counter is owned by
     * main.c (which adds n after every clear) so apply_score must not
     * touch it — otherwise lines would double-count. */
    idx = my_clamp(lines_cleared, 0, 4);
    b->score += my_mul(TABLE[idx], b->level + 1);
}

void board_update_level(Board *b)
{
    b->level = my_div(b->lines, 10);
}

/* ------------------------------------------------------------------ */
/*  TEST_BOARD — compile with -DTEST_BOARD                             */
/*  gcc -DTEST_BOARD -std=c99 -Wall -Wextra game/board.c game/piece.c */
/*       libs/math.c libs/memory.c libs/safe.c libs/keyboard.c        */
/*       libs/screen.c libs/string.c -o tboard && ./tboard             */
/* ------------------------------------------------------------------ */

#ifdef TEST_BOARD

int main(void)
{
    Board *b;
    Piece *p;
    int    c, cleared;

    mem_init();

    /* ---- Test 1: line clear ---- */
    b = board_new();

    /* Fill row 19 entirely with colour 1 */
    for (c = 0; c < BOARD_W; c++)
        b->cells[cell_index(19, c)] = 1;

    cleared = board_clear_lines(b);
    safe_assert(cleared == 1);

    /* Row 19 must now be all zeroes */
    for (c = 0; c < BOARD_W; c++)
        safe_assert(b->cells[cell_index(19, c)] == 0);

    fprintf(stdout, "Test 1 passed: single line clear.\n");

    /* ---- Test 2: collision detection ---- */
    /* Lock an O-piece at the top (row 0, col 3).
     * O-piece rot0 fills (1,1),(1,2),(2,1),(2,2) in its 4x4 box,
     * so board cells (1,4),(1,5),(2,4),(2,5) get filled. */
    p = piece_new(PIECE_O);
    p->row = 0;
    p->col = 3;
    board_lock(b, p);

    /* Verify the cells are occupied */
    safe_assert(board_at(b, 1, 4) == 1);
    safe_assert(board_at(b, 1, 5) == 1);
    safe_assert(board_at(b, 2, 4) == 1);
    safe_assert(board_at(b, 2, 5) == 1);

    /* Try placing another O-piece at the same position — must fail */
    safe_assert(board_can_place(b, p, 0, 0, 0) == 0);

    /* Shift it far enough away — must succeed */
    p->col = 0;
    p->row = 0;
    safe_assert(board_can_place(b, p, 0, 0, 0) == 1);

    fprintf(stdout, "Test 2 passed: collision detection.\n");

    /* ---- Test 3: scoring ---- */
    b->score = 0;
    b->level = 0;
    b->lines = 0;
    board_apply_score(b, 4);       /* Tetris at level 0 → 800 */
    safe_assert(b->score == 800);
    b->lines += 4;                  /* main.c owns the line counter */
    safe_assert(b->lines == 4);

    board_update_level(b);
    safe_assert(b->level == 0);    /* 4 lines → level 0 still */

    board_apply_score(b, 4);
    b->lines += 4;
    safe_assert(b->score == 1600);
    safe_assert(b->lines == 8);

    board_apply_score(b, 2);
    b->lines += 2;
    board_update_level(b);
    safe_assert(b->level == 1);    /* 10 lines → level 1 */

    fprintf(stdout, "Test 3 passed: scoring and levelling.\n");

    piece_free(p);
    board_free(b);

    fprintf(stdout, "All board tests passed.\n");
    return 0;
}

#endif
