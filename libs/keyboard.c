#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "keyboard.h"

/* Saved at kb_init() time; restored verbatim by kb_restore().
 * Static so it persists for the lifetime of the process. */
static struct termios g_orig_termios;

/* --- kb_init ---------------------------------------------------------------
 * Switch the terminal into raw, non-blocking mode:
 *
 *  ICANON off — disable line buffering; every keypress is available
 *               immediately without waiting for Enter.
 *  ECHO   off — suppress character echo so Tetris pieces are not obscured.
 *  VMIN=0     — read() returns immediately even if no data is ready.
 *  VTIME=0    — no inter-character timer; pairs with VMIN=0 for pure
 *               non-blocking behaviour.
 *
 * O_NONBLOCK on the file descriptor makes read() return EAGAIN (-1) rather
 * than blocking when the input queue is empty — essential for the game loop.
 *
 * kb_restore() MUST be called before exit; failing to do so leaves the
 * calling shell in raw mode (no echo, no line editing).
 * -------------------------------------------------------------------------- */
void kb_init(void)
{
    struct termios raw;
    int            flags;

    /* Snapshot the current settings so kb_restore() can replay them. */
    tcgetattr(STDIN_FILENO, &g_orig_termios);

    raw = g_orig_termios;

    /* Disable canonical mode and echo. */
    raw.c_lflag &= (unsigned int)~(ICANON | ECHO);

    /* VMIN=0 + VTIME=0: read() returns however many bytes are available
     * (including zero), giving us true non-blocking reads at the termios
     * layer independently of O_NONBLOCK. */
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* Also set O_NONBLOCK on the fd so read() returns -1/EAGAIN
     * rather than 0 on an empty buffer — makes the "no key" path
     * unambiguous: 0 could be a valid NUL character. */
    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

/* --- kb_restore ------------------------------------------------------------
 * Undo everything kb_init() did:
 *   1. Replay the original termios struct (re-enables ICANON and ECHO).
 *   2. Clear O_NONBLOCK so subsequent shell I/O behaves normally.
 *
 * Safe to call even before kb_init() — tcsetattr() will simply apply
 * whatever was in g_orig_termios (zeroed at program start on most platforms).
 * -------------------------------------------------------------------------- */
void kb_restore(void)
{
    int flags;

    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

/* --- kb_pressed ------------------------------------------------------------
 * Non-blocking key poll for the game loop.
 *
 * Returns:
 *   KEY_UP / KEY_DOWN / KEY_LEFT / KEY_RIGHT  for arrow keys
 *   the ASCII value of any other key
 *   -1 if no key is currently pressed
 *
 * Arrow key detection:
 *   ANSI terminals send a 3-byte escape sequence for each arrow key:
 *     ESC  [  A   → Up
 *     ESC  [  B   → Down
 *     ESC  [  C   → Right
 *     ESC  [  D   → Left
 *
 *   After reading the ESC (0x1B) byte we attempt two further reads.
 *   Because VMIN=0 + O_NONBLOCK is in effect, those reads return
 *   immediately.  In practice the three bytes always arrive together in
 *   the kernel buffer (the terminal sends them atomically), so the
 *   subsequent reads succeed.  If they fail (bytes not yet available)
 *   we fall through and return plain ESC so the caller can handle it.
 * -------------------------------------------------------------------------- */
int kb_pressed(void)
{
    unsigned char ch;
    unsigned char seq[2];

    /* Read one byte; returns -1 (errno=EAGAIN) or 0 if nothing available. */
    if (read(STDIN_FILENO, &ch, 1) <= 0)
        return -1;

    /* Plain ASCII or control character — pass straight through. */
    if (ch != 0x1B)
        return (int)ch;

    /* ESC received — try to consume the CSI sequence [ A/B/C/D. */
    if (read(STDIN_FILENO, &seq[0], 1) <= 0)
        return 0x1B; /* bare ESC, no sequence followed */

    if (seq[0] != '[')
        return 0x1B; /* not a CSI sequence we recognise */

    if (read(STDIN_FILENO, &seq[1], 1) <= 0)
        return 0x1B;

    switch (seq[1])
    {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return 0x1B; /* unknown escape sequence */
    }
}

/* --- kb_readline -----------------------------------------------------------
 * Blocking line reader — used for menus, name entry, etc., where raw
 * non-blocking reads would make character-by-character input awkward.
 *
 * Temporarily restores canonical + blocking mode so the kernel handles
 * line assembly for us, then switches back to raw mode when done.
 *
 * Reads at most (max - 1) characters; always NUL-terminates buf.
 * The newline itself is NOT stored in buf (mirrors fgets behaviour).
 * -------------------------------------------------------------------------- */
void kb_readline(char *buf, int max)
{
    struct termios cooked;
    int            flags;
    int            i;
    unsigned char  ch;

    if (max <= 0)
        return;

    /* --- Switch to canonical + blocking mode ----------------------------- */
    cooked = g_orig_termios; /* original settings already have ICANON|ECHO */
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    /* --- Read characters one at a time ----------------------------------- */
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

    /* --- Restore raw + non-blocking mode --------------------------------- */
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
