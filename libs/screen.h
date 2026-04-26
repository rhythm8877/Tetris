/*
 * libs/screen.h
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * Double-buffered terminal renderer.  Only scr_present() touches
 * stdout; all scr_put* functions write to the back buffer.
 */

#ifndef MY_SCREEN_H
#define MY_SCREEN_H

#define SCR_ROWS 30
#define SCR_COLS 80

void scr_init(void);
void scr_shutdown(void);
void scr_clear(void);
void scr_putchar(int row, int col, char ch);
void scr_puts(int row, int col, const char *s);
void scr_render_int(int row, int col, int n);
void scr_draw_border(int top, int left, int height, int width);
void scr_set_color(int color_id);
void scr_present(void);
void scr_force_full_redraw(void);

#endif
