#ifndef MY_KEYBOARD_H
#define MY_KEYBOARD_H

#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_LEFT  75
#define KEY_RIGHT 77

void kb_init(void);
void kb_restore(void);
int  kb_pressed(void);
void kb_readline(char *buf, int max);

#endif
