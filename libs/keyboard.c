#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "keyboard.h"

static struct termios g_orig_termios;
static int is_raw_mode = 0;

void kb_init(void)
{
    struct termios raw;
    int flags;

    tcgetattr(STDIN_FILENO, &g_orig_termios);

    raw = g_orig_termios;

    raw.c_lflag &= (unsigned int)~(ICANON | ECHO);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    is_raw_mode = 1;
}

void kb_restore(void)
{
    int flags;

    if (!is_raw_mode)
        return;
    is_raw_mode = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

int kb_pressed(void)
{
    unsigned char ch;
    unsigned char seq[2];

    if (read(STDIN_FILENO, &ch, 1) <= 0)
        return -1;

    if (ch != 0x1B)
        return (int)ch;

    if (read(STDIN_FILENO, &seq[0], 1) <= 0)
        return 0x1B;

    if (seq[0] != '[')
        return 0x1B;

    if (read(STDIN_FILENO, &seq[1], 1) <= 0)
        return 0x1B;

    switch (seq[1]){
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return 0x1B;
    }
}

void kb_readline(char *buf, int max)
{
    struct termios cooked;
    int            flags;
    int            i;
    unsigned char  ch;

    if (max <= 0)
        return;

    cooked = g_orig_termios;
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    i = 0;
    while (i < max - 1)
    {
        if (read(STDIN_FILENO, &ch, 1) <= 0)
            break;
        if (ch == '\n' || ch == '\r')
            break;
        buf[i++] = (char)ch;
    }
    buf[i] = '\0';

    {
        struct termios raw;

        raw = g_orig_termios;
        raw.c_lflag &= (unsigned int)~(ICANON | ECHO);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}
