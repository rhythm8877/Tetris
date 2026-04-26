/*
 * libs/screen.c
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * Double-buffered terminal renderer.  Two pairs of buffers (char +
 * colour attribute) are allocated through libs/memory.h.  scr_present()
 * diffs back vs. front, emits cursor-move + colour-change + char only
 * for cells that actually changed, then fflush(stdout) once.
 *
 * This eliminates the "stacked board" bug caused by the Phase 1
 * \033[2J\033[H-every-frame approach.
 */

#include "screen.h"
#include "string.h"
#include "math.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>    /* getenv only */

/* ------------------------------------------------------------------ */
/*  Static state                                                       */
/* ------------------------------------------------------------------ */

static char          *back       = 0;
static char          *front      = 0;
static unsigned char *attr_back  = 0;
static unsigned char *attr_front = 0;

static int cur_color     = 0;
static int color_enabled = 1;
static int force_full    = 0;

#define BUF_SIZE (SCR_ROWS * SCR_COLS)    /* constant, no my_mul needed */

/* ------------------------------------------------------------------ */
/*  scr_init                                                           */
/* ------------------------------------------------------------------ */

void scr_init(void)
{
    const char *env;
    int         i;

    back       = (char *)my_alloc(BUF_SIZE);
    front      = (char *)my_alloc(BUF_SIZE);
    attr_back  = (unsigned char *)my_alloc(BUF_SIZE);
    attr_front = (unsigned char *)my_alloc(BUF_SIZE);

    if (!back || !front || !attr_back || !attr_front)
        return;

    for (i = 0; i < BUF_SIZE; i++)
    {
        back[i]       = ' ';
        front[i]      = ' ';
        attr_back[i]  = 0;
        attr_front[i] = 0;
    }

    cur_color  = 0;
    force_full = 0;

    env = getenv("COLOR");
    color_enabled = !(env && env[0] == '0' && env[1] == '\0');

    fputs("\033[2J\033[H", stdout);   /* one-time full clear  */
    fputs("\033[?25l", stdout);       /* hide cursor          */
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  scr_shutdown                                                       */
/* ------------------------------------------------------------------ */

void scr_shutdown(void)
{
    fputs("\033[?25h", stdout);       /* show cursor          */
    fputs("\033[0m", stdout);         /* reset attributes     */
    fputs("\033[2J\033[H", stdout);   /* clear screen         */
    fflush(stdout);

    my_dealloc(back);
    my_dealloc(front);
    my_dealloc(attr_back);
    my_dealloc(attr_front);

    back       = 0;
    front      = 0;
    attr_back  = 0;
    attr_front = 0;
}

/* ------------------------------------------------------------------ */
/*  scr_clear                                                          */
/* ------------------------------------------------------------------ */

void scr_clear(void)
{
    int i;

    if (!back)
        return;

    for (i = 0; i < BUF_SIZE; i++)
    {
        back[i]      = ' ';
        attr_back[i] = 0;
    }

    force_full = 1;
}

/* ------------------------------------------------------------------ */
/*  scr_putchar / scr_puts / scr_render_int                           */
/* ------------------------------------------------------------------ */

void scr_putchar(int row, int col, char ch)
{
    int idx;

    if (row < 0 || row >= SCR_ROWS || col < 0 || col >= SCR_COLS)
        return;

    idx           = my_mul(row, SCR_COLS) + col;
    back[idx]     = ch;
    attr_back[idx] = (unsigned char)cur_color;
}

void scr_puts(int row, int col, const char *s)
{
    int i;

    for (i = 0; s[i] != '\0'; i++)
        scr_putchar(row, col + i, s[i]);
}

void scr_render_int(int row, int col, int n)
{
    char buf[12];

    my_itoa(n, buf);
    scr_puts(row, col, buf);
}

/* ------------------------------------------------------------------ */
/*  scr_draw_border                                                    */
/* ------------------------------------------------------------------ */

void scr_draw_border(int top, int left, int height, int width)
{
    int row;
    int col;
    int bottom;
    int right;

    bottom = top  + height - 1;
    right  = left + width  - 1;

    scr_putchar(top, left, '+');
    for (col = left + 1; col < right; col++)
        scr_putchar(top, col, '-');
    if (width > 1)
        scr_putchar(top, right, '+');

    if (height > 1)
    {
        scr_putchar(bottom, left, '+');
        for (col = left + 1; col < right; col++)
            scr_putchar(bottom, col, '-');
        if (width > 1)
            scr_putchar(bottom, right, '+');
    }

    for (row = top + 1; row < bottom; row++)
    {
        scr_putchar(row, left,  '|');
        if (width > 1)
            scr_putchar(row, right, '|');
    }
}

/* ------------------------------------------------------------------ */
/*  scr_set_color                                                      */
/* ------------------------------------------------------------------ */

void scr_set_color(int color_id)
{
    if (!color_enabled)
        return;

    cur_color = color_id;
}

/* ------------------------------------------------------------------ */
/*  scr_present — diff back vs front, emit only changes                */
/* ------------------------------------------------------------------ */

void scr_present(void)
{
    int row;
    int col;
    int idx;
    int last_row;
    int last_col;
    int emitted_color;

    if (!back)
        return;

    last_row      = -1;
    last_col      = -1;
    emitted_color = -1;

    for (row = 0; row < SCR_ROWS; row++)
    {
        for (col = 0; col < SCR_COLS; col++)
        {
            idx = my_mul(row, SCR_COLS) + col;

            if (!force_full
                && back[idx] == front[idx]
                && attr_back[idx] == attr_front[idx])
            {
                continue;
            }

            /* Cursor-move when not consecutive on the same row.
             * printf with %d is acceptable here — this is the frame-
             * flush path, not score arithmetic. */
            if (last_row != row || last_col != col)
                printf("\033[%d;%dH", row + 1, col + 1);

            if (color_enabled && (int)attr_back[idx] != emitted_color)
            {
                emitted_color = (int)attr_back[idx];
                if (emitted_color == 0)
                    fputs("\033[0m", stdout);
                else
                    printf("\033[%dm", 30 + emitted_color);
            }

            putchar(back[idx]);

            front[idx]      = back[idx];
            attr_front[idx] = attr_back[idx];

            last_row = row;
            last_col = col + 1;
        }
    }

    force_full = 0;
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  scr_force_full_redraw                                              */
/* ------------------------------------------------------------------ */

void scr_force_full_redraw(void)
{
    force_full = 1;
}
