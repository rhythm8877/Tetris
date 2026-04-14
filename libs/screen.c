#include <stdio.h>
#include "screen.h"
#include "string.h"

void scr_clear(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

void scr_move(int row, int col)
{
    printf("\033[%d;%dH", row, col);
}

void scr_putchar(int row, int col, char ch)
{
    scr_move(row, col);
    putchar(ch);
}

void scr_puts(int row, int col, const char *s)
{
    int i;

    scr_move(row, col);
    for (i = 0; s[i] != '\0'; i++)
        putchar(s[i]);
}

void scr_render_int(int row, int col, int n)
{
    char buf[12];

    my_itoa(n, buf);
    scr_puts(row, col, buf);
    fflush(stdout);
}

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

    fflush(stdout);
}
