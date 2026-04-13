#ifndef MY_SCREEN_H
#define MY_SCREEN_H

void scr_clear(void);
void scr_move(int row, int col);
void scr_putchar(char c);
void scr_puts(const char *s);
void scr_render_int(int n);

#endif /* MY_SCREEN_H */
