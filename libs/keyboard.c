#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "keyboard.h"

void kb_init(void)                   {}
void kb_restore(void)                {}
int  kb_pressed(void)                { return 0; }
void kb_readline(char *buf, int max) { (void)buf; (void)max; }
