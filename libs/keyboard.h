#ifndef MY_KEYBOARD_H
#define MY_KEYBOARD_H

/*
 * Arrow keys reuse legacy DOS scancodes as sentinels. Note these collide
 * with ASCII letters 'H' (72), 'P' (80), 'K' (75), 'M' (77) — game code
 * never uses those letters as commands so the overlap is harmless.
 */
#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_LEFT  75
#define KEY_RIGHT 77

/*
 * Phase 2 additions — these intentionally equal their ASCII codes so
 * kb_pressed() can return them directly.
 */
#define KEY_SPACE 32
#define KEY_ENTER 10
#define KEY_ESC   27

void kb_init(void);
void kb_restore(void);
int  kb_pressed(void);
void kb_readline(char *buf, int max);

#endif
