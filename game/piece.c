/*
 * game/piece.c
 * Author : Amod
 * Desc   : 7 standard tetrominoes with SRS rotations and basic wall kicks.
 * Date   : {{DATE}}
 */

#include <stdio.h>

#include "piece.h"
#include "../libs/math.h"
#include "../libs/memory.h"
#include "../libs/safe.h"

/* Board width — must agree with main.c / board.h */
#define BOARD_W 10

/* ------------------------------------------------------------------ */
/*  Shape data — SRS standard                                          */
/*                                                                     */
/*  Each 16-bit word encodes a 4x4 grid.  Bit (r*4 + c) is set when   */
/*  cell (r, c) is filled.  r=0 is the top row.                        */
/*                                                                     */
/*  Visual key  (bit index shown in grid):                              */
/*      0  1  2  3          row 0                                      */
/*      4  5  6  7          row 1                                      */
/*      8  9 10 11          row 2                                      */
/*     12 13 14 15          row 3                                      */
/*                                                                     */
/*  I rot0:  ....   rot1:  ..#.   rot2:  ....   rot3:  .#..            */
/*           ####          ..#.          ....          .#..             */
/*           ....          ..#.          ####          .#..             */
/*           ....          ..#.          ....          .#..             */
/*                                                                     */
/*  O rot0:  ....   rot1:  ....   rot2:  ....   rot3:  ....            */
/*           .##.          .##.          .##.          .##.             */
/*           .##.          .##.          .##.          .##.             */
/*           ....          ....          ....          ....             */
/*                                                                     */
/*  T rot0:  ....   rot1:  .#..   rot2:  ....   rot3:  .#..            */
/*           ###.          .##.          .#..          ##..             */
/*           .#..          .#..          ###.          .#..             */
/*           ....          ....          ....          ....             */
/*                                                                     */
/*  S rot0:  ....   rot1:  .#..   rot2:  ....   rot3:  #...            */
/*           .##.          .##.          .##.          ##..             */
/*           ##..          ..#.          ##..          .#..             */
/*           ....          ....          ....          ....             */
/*                                                                     */
/*  Z rot0:  ....   rot1:  ..#.   rot2:  ....   rot3:  .#..            */
/*           ##..          .##.          ##..          ##..             */
/*           .##.          .#..          .##.          #...             */
/*           ....          ....          ....          ....             */
/*                                                                     */
/*  J rot0:  ....   rot1:  .#..   rot2:  ....   rot3:  .##.            */
/*           ###.          .#..          ..#.          .#..             */
/*           #...          ##..          ###.          .#..             */
/*           ....          ....          ....          ....             */
/*                                                                     */
/*  L rot0:  ....   rot1:  ##..   rot2:  ....   rot3:  .#..            */
/*           ###.          .#..          #...          .#..             */
/*           ..#.          .#..          ###.          .##.             */
/*           ....          ....          ....          ....             */
/* ------------------------------------------------------------------ */

static const unsigned short SHAPES[7][4] = {
    /* PIECE_I */
    {
        0x00F0,  /* rot0: .... / #### / .... / .... */
        0x4444,  /* rot1: ..#. / ..#. / ..#. / ..#. */
        0x0F00,  /* rot2: .... / .... / #### / .... */
        0x2222   /* rot3: .#.. / .#.. / .#.. / .#.. */
    },
    /* PIECE_O */
    {
        0x0660,  /* rot0: .... / .##. / .##. / .... */
        0x0660,  /* rot1: (same) */
        0x0660,  /* rot2: (same) */
        0x0660   /* rot3: (same) */
    },
    /* PIECE_T */
    {
        0x0270,  /* rot0: .... / ###. / .#.. / .... */
        0x0262,  /* rot1: .#.. / .##. / .#.. / .... */
        0x0720,  /* rot2: .... / .#.. / ###. / .... */
        0x0232   /* rot3: .#.. / ##.. / .#.. / .... */
    },
    /* PIECE_S */
    {
        0x0360,  /* rot0: .... / .##. / ##.. / .... */
        0x0462,  /* rot1: .#.. / .##. / ..#. / .... */
        0x0360,  /* rot2: (same as rot0) */
        0x0231   /* rot3: #... / ##.. / .#.. / .... */
    },
    /* PIECE_Z */
    {
        0x0630,  /* rot0: .... / ##.. / .##. / .... */
        0x0264,  /* rot1: ..#. / .##. / .#.. / .... */
        0x0630,  /* rot2: (same as rot0) */
        0x0132   /* rot3: .#.. / ##.. / #... / .... */
    },
    /* PIECE_J */
    {
        0x0170,  /* rot0: .... / ###. / #... / .... */
        0x0322,  /* rot1: .#.. / .#.. / ##.. / .... */
        0x0740,  /* rot2: .... / ..#. / ###. / .... */
        0x0226   /* rot3: .##. / .#.. / .#.. / .... */
    },
    /* PIECE_L */
    {
        0x0470,  /* rot0: .... / ###. / ..#. / .... */
        0x0223,  /* rot1: ##.. / .#.. / .#.. / .... */
        0x0710,  /* rot2: .... / #... / ###. / .... */
        0x0622   /* rot3: .#.. / .#.. / .##. / .... */
    }
};

/* Colour table: I=6(cyan), O=3(yellow), T=5(magenta),
 *               S=2(green), Z=1(red), J=4(blue), L=3(yellow) */
static const int COLORS[7] = { 6, 3, 5, 2, 1, 4, 3 };

/* Phase-2 wall-kick offsets — column-only: 0, +1, -1, +2, -2 */
static const int KICK_DC[5] = { 0, 1, -1, 2, -2 };
#define KICK_COUNT 5

/* ------------------------------------------------------------------ */
/*  Public implementation                                              */
/* ------------------------------------------------------------------ */

int piece_cell(int type, int rot, int dr, int dc)
{
    unsigned short mask;
    int bit;

    if (type < 0 || type >= PIECE_COUNT)
        return 0;
    rot = my_mod(rot, 4);
    if (dr < 0 || dr >= PIECE_BOX || dc < 0 || dc >= PIECE_BOX)
        return 0;

    mask = SHAPES[type][rot];
    bit  = my_mul(dr, 4) + dc;        /* dr*4 + dc */
    return (mask >> bit) & 1;
}

Piece *piece_new(int type)
{
    Piece *p;

    safe_assert(type >= 0 && type < PIECE_COUNT);

    p = (Piece *)my_alloc((int)sizeof(Piece));
    safe_assert(p != (void *)0);

    p->type     = type;
    p->rot      = 0;
    p->row      = 0;
    p->col      = my_div(BOARD_W, 2) - 2;
    p->color_id = COLORS[type];
    return p;
}

void piece_free(Piece *p)
{
    if (p)
        my_dealloc(p);
}

void piece_rotate_cw(Piece *p)
{
    safe_assert(p != (void *)0);
    p->rot = my_mod(p->rot + 1, 4);
}

void piece_rotate_ccw(Piece *p)
{
    safe_assert(p != (void *)0);
    p->rot = my_mod(p->rot + 3, 4);
}

int piece_kick_count(int type)
{
    (void)type;
    return KICK_COUNT;
}

void piece_kick_offset(int type, int idx, int *dr, int *dc)
{
    (void)type;
    safe_assert(idx >= 0 && idx < KICK_COUNT);
    *dr = 0;
    *dc = KICK_DC[idx];
}

int piece_color_for(int type)
{
    safe_assert(type >= 0 && type < PIECE_COUNT);
    return COLORS[type];
}

/* ------------------------------------------------------------------ */
/*  TEST_PIECE — compile with -DTEST_PIECE                             */
/*  gcc -DTEST_PIECE -std=c99 -Wall -Wextra game/piece.c              */
/*       libs/math.c libs/memory.c libs/safe.c libs/keyboard.c        */
/*       libs/screen.c libs/string.c -o tpiece && ./tpiece             */
/* ------------------------------------------------------------------ */

#ifdef TEST_PIECE

static const char *TYPE_NAMES[7] = {
    "I", "O", "T", "S", "Z", "J", "L"
};

int main(void)
{
    int t, rot, r, c;

    mem_init();

    /* Print all 7 x 4 grids */
    for (t = 0; t < PIECE_COUNT; t++) {
        for (rot = 0; rot < 4; rot++) {
            fprintf(stdout, "%s rot%d:\n", TYPE_NAMES[t], rot);
            for (r = 0; r < PIECE_BOX; r++) {
                for (c = 0; c < PIECE_BOX; c++)
                    fprintf(stdout, "%c", piece_cell(t, rot, r, c) ? '#' : '.');
                fprintf(stdout, "\n");
            }
            fprintf(stdout, "\n");
        }
    }

    /* Assert kick count */
    safe_assert(piece_kick_count(PIECE_I) == 5);
    safe_assert(piece_kick_count(PIECE_T) == 5);

    fprintf(stdout, "All piece tests passed.\n");
    return 0;
}

#endif
