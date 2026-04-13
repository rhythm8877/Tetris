#ifndef MY_KEYBOARD_H
#define MY_KEYBOARD_H

#define KEY_UP    1000
#define KEY_DOWN  1001
#define KEY_LEFT  1002
#define KEY_RIGHT 1003

void kb_init(void);
void kb_restore(void);
int  kb_pressed(void);
void kb_readline(char *buf, int max);

#endif /* MY_KEYBOARD_H */
