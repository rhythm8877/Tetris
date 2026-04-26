/*
 * game/board.h
 * Author : Amod
 * Desc   : Playfield state — collision, locking, line clears, scoring.
 * Date   : {{DATE}}
 */

#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include "piece.h"

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */

#define BOARD_W 10
#define BOARD_H 20

/* ------------------------------------------------------------------ */
/*  Types                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int *cells;            /* BOARD_W * BOARD_H ints; 0 = empty, 1..7 = colour id */
    int  score;
    int  level;
    int  lines;
} Board;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

Board *board_new(void);
void   board_free(Board *b);

int    board_at(const Board *b, int r, int c);
int    board_can_place(const Board *b, const Piece *p,
                       int drow, int dcol, int drot);
void   board_lock(Board *b, const Piece *p);
int    board_clear_lines(Board *b);
void   board_apply_score(Board *b, int lines_cleared);
void   board_update_level(Board *b);

#endif
