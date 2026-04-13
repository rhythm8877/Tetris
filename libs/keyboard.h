#ifndef MY_KEYBOARD_H
#define MY_KEYBOARD_H

/* Sentinel values returned by kb_pressed() for arrow keys.
 * All are above 127 to avoid collision with any ASCII character.
 * The numeric values match the DOS/BIOS scan-code convention so they are
 * easy to remember, but the actual detection uses ANSI escape sequences. */
#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_LEFT  75
#define KEY_RIGHT 77

void kb_init(void);
void kb_restore(void);
int  kb_pressed(void);
void kb_readline(char *buf, int max);

#endif /* MY_KEYBOARD_H */
