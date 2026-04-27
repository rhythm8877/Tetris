/*
 * libs/safe.h
 * Author : Amod
 * Date   : 2026-04-27
 *
 * Centralised halt/error handling and input sanitisation. Owns the
 * Security rubric for Phase 2:
 *   - safe_panic / safe_assert_fail   → fatal exit with terminal restored
 *   - safe_assert(cond)               → checked-invariant macro
 *   - SAFE_PANIC(msg)                 → call-site-tagged fatal exit
 *   - safe_sanitize_name()            → in-place strip + fallback
 *   - safe_alloc_or_panic()           → my_alloc wrapped with fatal-on-NULL
 */

#ifndef MY_SAFE_H
#define MY_SAFE_H

void safe_panic(const char *msg, const char *file, int line);
void safe_assert_fail(const char *expr, const char *file, int line);

int  safe_sanitize_name(char *name, int max_len);
int  safe_alloc_or_panic(void **out, int size, const char *what);

#define safe_assert(cond) \
    do { if (!(cond)) safe_assert_fail(#cond, __FILE__, __LINE__); } while (0)

#define SAFE_PANIC(msg) safe_panic((msg), __FILE__, __LINE__)

#endif
