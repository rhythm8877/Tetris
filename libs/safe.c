/*
 * libs/safe.c
 * Author : Amod
 * Date   : 2026-04-27
 *
 * Implementation of the safety / halt module. See safe.h.
 *
 * Note on terminal clearing at panic time:
 *   The Phase 2 screen.c is double-buffered, so calling scr_clear()
 *   only writes ' ' into the back buffer — nothing reaches the
 *   terminal until scr_present() runs, and we are about to _exit(1)
 *   without ever calling it. The original Phase 1 spec wanted the
 *   scrollback flushed, so we emit the ANSI clear-and-home sequence
 *   directly to stderr. Same end result, and avoids the misleading
 *   no-op call.
 */

#include <stdio.h>
#include <unistd.h>

#include "safe.h"
#include "string.h"
#include "memory.h"
#include "keyboard.h"

void safe_panic(const char *msg, const char *file, int line)
{
    /* Restore cooked, blocking, echoing terminal so the FATAL line is
     * actually visible and the shell prompt that follows works. */
    kb_restore();

    /* Clear screen + home cursor. See header comment for rationale. */
    fprintf(stderr, "\033[2J\033[H");
    fprintf(stderr, "[FATAL] %s @ %s:%d\n",
            (msg  != 0) ? msg  : "(null)",
            (file != 0) ? file : "(null)",
            line);

    /* _exit() (not exit()) so atexit handlers do not run a second
     * time over a possibly-corrupt heap. */
    _exit(1);
}

void safe_assert_fail(const char *expr, const char *file, int line)
{
    safe_panic(expr, file, line);
}

int safe_sanitize_name(char *name, int max_len)
{
    int  read_idx;
    int  write_idx;
    char c;

    if (name == 0 || max_len <= 0)
        return 0;

    /* In-place strip. Anything that is not [A-Za-z0-9_-] is dropped;
     * remaining chars shift left. Stops at original null terminator
     * or after max_len - 1 kept chars. */
    read_idx  = 0;
    write_idx = 0;
    while (name[read_idx] != '\0' && write_idx < max_len - 1) {
        c = name[read_idx];
        if (my_isalnum((int)(unsigned char)c) || c == '_' || c == '-') {
            name[write_idx] = c;
            write_idx++;
        }
        read_idx++;
    }
    name[write_idx] = '\0';

    /* Empty after stripping → fall back to "Anonymous". */
    if (write_idx == 0) {
        my_strncpy(name, "Anonymous", max_len);
        return my_strlen(name);
    }

    return write_idx;
}

int safe_alloc_or_panic(void **out, int size, const char *what)
{
    char  buf[64];
    int   n;
    int   i;
    void *p;

    if (out == 0)
        SAFE_PANIC("safe_alloc_or_panic called with NULL out");

    p = my_alloc(size);
    if (p != 0) {
        *out = p;
        return 0;
    }

    /* Build "RAM exhausted: <what>" in a stack buffer. We have no
     * sprintf in scope (string.h / math.h are limited), so concat by
     * my_strncpy + my_strlen + manual byte loop. */
    my_strncpy(buf, "RAM exhausted: ", (int)sizeof(buf));
    n = my_strlen(buf);
    if (what != 0) {
        for (i = 0; what[i] != '\0' && n < (int)sizeof(buf) - 1; i++)
            buf[n++] = what[i];
    }
    buf[n] = '\0';

    SAFE_PANIC(buf);
    return -1; /* unreachable; SAFE_PANIC does not return */
}
