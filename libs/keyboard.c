/*
 * libs/keyboard.c
 * Author : Rhythm
 * Phase  : 2 (extended from Phase 1)
 *
 * Manual test plan
 *   Compile: gcc -DTEST_KEYBOARD libs/keyboard.c -o tkb && ./tkb
 *   Expect:
 *     UP arrow    -> 72  (KEY_UP)
 *     DOWN arrow  -> 80  (KEY_DOWN)
 *     LEFT arrow  -> 75  (KEY_LEFT)
 *     RIGHT arrow -> 77  (KEY_RIGHT)
 *     SPACE       -> 32  (KEY_SPACE)
 *     ENTER       -> 10  (KEY_ENTER)   // CR (13) is normalised to LF (10)
 *     ESC alone   -> 27  (KEY_ESC)     // see comment in kb_pressed()
 *     q           -> exits the loop
 */

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

    /* Idempotent: safe to call multiple times (atexit + manual). */
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

    if (ch != 0x1B) {
        /* Some terminals emit CR (13) on ENTER. Normalise to LF (10) so
         * callers can compare against KEY_ENTER unconditionally. */
        if (ch == 13)
            return KEY_ENTER;
        return (int)ch;
    }

    /* ESC was pressed. If it stands alone, the next non-blocking read
     * returns -1 immediately (VMIN=VTIME=0, no buffered bytes), so each
     * of the three "return KEY_ESC" lines below is the fall-through path
     * that delivers a bare ESC up to the caller. */
    if (read(STDIN_FILENO, &seq[0], 1) <= 0)
        return KEY_ESC;

    if (seq[0] != '[')
        return KEY_ESC;

    if (read(STDIN_FILENO, &seq[1], 1) <= 0)
        return KEY_ESC;

    switch (seq[1]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return KEY_ESC;
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
        /* Accept both LF and CR as line terminators — different terminals
         * deliver one or the other depending on icrnl/inlcr settings. */
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

#ifdef TEST_KEYBOARD
#include <stdio.h>

int main(void)
{
    int k;

    kb_init();
    printf("kb test: press keys; q to quit\r\n");
    fflush(stdout);

    for (;;) {
        k = kb_pressed();
        if (k == -1) {
            usleep(10000);
            continue;
        }
        if (k == 'q') {
            printf("quit\r\n");
            break;
        }
        switch (k) {
            case KEY_UP:    printf("KEY_UP     (%d)\r\n", k); break;
            case KEY_DOWN:  printf("KEY_DOWN   (%d)\r\n", k); break;
            case KEY_LEFT:  printf("KEY_LEFT   (%d)\r\n", k); break;
            case KEY_RIGHT: printf("KEY_RIGHT  (%d)\r\n", k); break;
            case KEY_SPACE: printf("KEY_SPACE  (%d)\r\n", k); break;
            case KEY_ENTER: printf("KEY_ENTER  (%d)\r\n", k); break;
            case KEY_ESC:   printf("KEY_ESC    (%d)\r\n", k); break;
            default:
                if (k >= 32 && k < 127)
                    printf("char='%c'   (%d)\r\n", k, k);
                else
                    printf("byte=?     (%d)\r\n", k);
                break;
        }
        fflush(stdout);
    }

    kb_restore();
    return 0;
}
#endif
