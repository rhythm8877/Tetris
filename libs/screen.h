#ifndef MY_SCREEN_H
#define MY_SCREEN_H

void scr_clear(void);
void scr_move(int row, int col);
void scr_putchar(int row, int col, char ch);
void scr_puts(int row, int col, const char *s);
void scr_render_int(int row, int col, int n);
void scr_draw_border(int top, int left, int height, int width);

#endif
