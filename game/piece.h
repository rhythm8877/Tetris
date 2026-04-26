/*
 * game/piece.h
 * Author : Amod
 * Desc   : 7 standard tetrominoes with SRS rotations and basic wall kicks.
 * Date   : {{DATE}}
 */

#ifndef GAME_PIECE_H
#define GAME_PIECE_H

/* ------------------------------------------------------------------ */
/*  Types                                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    PIECE_I = 0,
    PIECE_O,
    PIECE_T,
    PIECE_S,
    PIECE_Z,
    PIECE_J,
    PIECE_L,
    PIECE_COUNT            /* 7 */
} PieceType;

typedef struct {
    int type;              /* PieceType                                */
    int rot;               /* 0..3                                     */
    int row;               /* top-left row of 4x4 bounding box        */
    int col;               /* top-left col of 4x4 bounding box        */
    int color_id;          /* matches screen.h colour table            */
} Piece;

#define PIECE_BOX  4       /* every shape lives in a 4x4 cell grid    */

#define PIECE_KICK_FAIL  -999

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

Piece *piece_new(int type);
void   piece_free(Piece *p);

int    piece_cell(int type, int rot, int dr, int dc);

void   piece_rotate_cw(Piece *p);
void   piece_rotate_ccw(Piece *p);

int    piece_kick_count(int type);
void   piece_kick_offset(int type, int idx, int *dr, int *dc);

int    piece_color_for(int type);

#endif
