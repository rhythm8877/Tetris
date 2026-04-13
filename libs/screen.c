#include <stdio.h>
#include "screen.h"
#include "string.h"

/* --- scr_clear ------------------------------------------------------------
 * Erase the entire terminal and home the cursor in one escape sequence:
 *   \033[2J   — erase whole screen
 *   \033[H    — move cursor to row 1, col 1
 * Using a single printf keeps the two codes atomic from the terminal's view.
 * -------------------------------------------------------------------------- */
void scr_clear(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* --- scr_move -------------------------------------------------------------
 * Position the cursor at (row, col) using the ANSI CSI cursor-position code.
 * Row and column are 1-based in ANSI; callers of this library also use 1-based
 * coordinates so no adjustment is needed.
 * -------------------------------------------------------------------------- */
void scr_move(int row, int col)
{
    printf("\033[%d;%dH", row, col);
}

/* --- scr_putchar ----------------------------------------------------------
 * Move the cursor to (row, col), then emit a single character.
 * Uses putchar() rather than printf() to avoid format-string overhead for
 * the hot path that draws every Tetromino cell individually.
 * -------------------------------------------------------------------------- */
void scr_putchar(int row, int col, char ch)
{
    scr_move(row, col);
    putchar(ch);
}

/* --- scr_puts -------------------------------------------------------------
 * Move the cursor to (row, col) once, then stream the string character by
 * character with putchar().  A single scr_move avoids re-issuing the escape
 * sequence for every character — the terminal advances the cursor naturally
 * after each printed character.
 * -------------------------------------------------------------------------- */
void scr_puts(int row, int col, const char *s)
{
    int i;

    scr_move(row, col);
    for (i = 0; s[i] != '\0'; i++)
        putchar(s[i]);
}

/* --- scr_render_int -------------------------------------------------------
 * Convert n to a decimal string via my_itoa() (no printf("%d") allowed),
 * then hand it to scr_puts() for placement.  fflush() is called afterwards
 * so score/level updates appear immediately — terminal output is line-buffered
 * by default and would otherwise be held until a newline.
 *
 * buf is sized to 12 bytes: sign + 10 digits + '\0' covers all 32-bit values.
 * -------------------------------------------------------------------------- */
void scr_render_int(int row, int col, int n)
{
    char buf[12];

    my_itoa(n, buf);
    scr_puts(row, col, buf);
    fflush(stdout);
}

/* --- scr_draw_border ------------------------------------------------------
 * Draw a hollow rectangle at the given position using '+', '-', '|':
 *
 *   (top, left)  +-----------+  (top, left+width-1)
 *                |           |
 *                +-----------+  (top+height-1, left+width-1)
 *
 * Corners  : '+'  at the four corner cells
 * Top/bottom edges: '-'  from col left+1 to left+width-2
 * Left/right edges: '|'  from row top+1  to top+height-2
 *
 * All dimensions are dynamic — no hard-coded sizes.
 * Degenerate cases: height or width < 2 means no interior exists; the
 * function still draws whatever fits (corners only if both are exactly 1).
 * -------------------------------------------------------------------------- */
void scr_draw_border(int top, int left, int height, int width)
{
    int row;
    int col;
    int bottom;
    int right;

    bottom = top  + height - 1;
    right  = left + width  - 1;

    /* Top edge */
    scr_putchar(top, left, '+');
    for (col = left + 1; col < right; col++)
        scr_putchar(top, col, '-');
    if (width > 1)
        scr_putchar(top, right, '+');

    /* Bottom edge (only if height > 1) */
    if (height > 1)
    {
        scr_putchar(bottom, left, '+');
        for (col = left + 1; col < right; col++)
            scr_putchar(bottom, col, '-');
        if (width > 1)
            scr_putchar(bottom, right, '+');
    }

    /* Left and right edges */
    for (row = top + 1; row < bottom; row++)
    {
        scr_putchar(row, left,  '|');
        if (width > 1)
            scr_putchar(row, right, '|');
    }

    fflush(stdout);
}
