#include <stdio.h>
#include "screen.h"

void scr_clear(void)            {}
void scr_move(int row, int col) { (void)row; (void)col; }
void scr_putchar(char c)        { (void)c; }
void scr_puts(const char *s)    { (void)s; }
void scr_render_int(int n)      { (void)n; }
