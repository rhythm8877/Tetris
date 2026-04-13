#include "math.h"

#ifdef TEST_MATH
#include <stdio.h>
#endif

/* --- my_abs ---------------------------------------------------------------
 * Returns the absolute value of a by negating negatives.
 * No branching trick: straightforward sign check to keep it readable.
 * Used internally by my_mul and my_div to work on magnitudes only.
 * -------------------------------------------------------------------------- */
int my_abs(int a)
{
    return (a < 0) ? -a : a;
}

/* --- my_mul ---------------------------------------------------------------
 * Multiply by repeated addition on the magnitudes, then fix the sign.
 * Sign rule: result is negative iff exactly one operand is negative.
 * Special case: either operand is 0 → return 0 immediately.
 * -------------------------------------------------------------------------- */
int my_mul(int a, int b)
{
    int neg;
    int ua;
    int ub;
    int result;
    int i;

    if (a == 0 || b == 0)
        return 0;

    neg = ((a < 0) != (b < 0)); /* XOR sign bits */
    ua  = my_abs(a);
    ub  = my_abs(b);

    result = 0;
    for (i = 0; i < ub; i++)
        result += ua;

    return neg ? -result : result;
}

/* --- my_div ---------------------------------------------------------------
 * Integer division by repeated subtraction on the magnitudes.
 * Sign rule: same XOR rule as my_mul — one negative → negative quotient.
 * Division by zero is a safe no-op: returns 0 (caller must not rely on it
 * for meaningful results, but the program will not crash or loop forever).
 * -------------------------------------------------------------------------- */
int my_div(int a, int b)
{
    int neg;
    int ua;
    int ub;
    int quotient;

    if (b == 0)
        return 0; /* safe guard — undefined in C, we return 0 */

    neg = ((a < 0) != (b < 0));
    ua  = my_abs(a);
    ub  = my_abs(b);

    quotient = 0;
    while (ua >= ub)
    {
        ua -= ub;
        quotient++;
    }

    return neg ? -quotient : quotient;
}

/* --- my_mod ---------------------------------------------------------------
 * Computes a mod b as: a - (a/b)*b
 * Delegates entirely to my_div and my_mul so no % operator is used.
 * Division by zero guard is inherited from my_div (returns 0).
 * The sign of the result matches the sign of a (C99 truncation semantics).
 * -------------------------------------------------------------------------- */
int my_mod(int a, int b)
{
    if (b == 0)
        return 0;
    return a - my_mul(my_div(a, b), b);
}

/* --- my_max ---------------------------------------------------------------
 * Returns the larger of two ints via a single ternary comparison.
 * -------------------------------------------------------------------------- */
int my_max(int a, int b)
{
    return (a > b) ? a : b;
}

/* --- my_min ---------------------------------------------------------------
 * Returns the smaller of two ints via a single ternary comparison.
 * -------------------------------------------------------------------------- */
int my_min(int a, int b)
{
    return (a < b) ? a : b;
}

/* --- my_clamp -------------------------------------------------------------
 * Restricts val to the closed interval [lo, hi].
 * If lo > hi the behaviour is undefined by convention — caller's problem.
 * Used by the Tetris engine to keep pieces inside the board walls.
 * -------------------------------------------------------------------------- */
int my_clamp(int val, int lo, int hi)
{
    return my_max(lo, my_min(val, hi));
}

/* ==========================================================================
 * Self-contained test block — compile with -DTEST_MATH and run the binary.
 *
 *   gcc -Wall -Wextra -DTEST_MATH -Ilibs libs/math.c -o math_test && ./math_test
 *
 * ========================================================================== */
#ifdef TEST_MATH

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"

static int g_tests  = 0;
static int g_passed = 0;

static void check(const char *label, int got, int expected)
{
    g_tests++;
    if (got == expected)
    {
        printf("[%s] %s  (got %d)\n", PASS, label, got);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got %d, expected %d)\n", FAIL, label, got, expected);
    }
}

int main(void)
{
    printf("=== my_abs ===\n");
    check("abs(0)",    my_abs(0),    0);
    check("abs(5)",    my_abs(5),    5);
    check("abs(-5)",   my_abs(-5),   5);
    check("abs(-100)", my_abs(-100), 100);

    printf("\n=== my_mul ===\n");
    check("mul(0, 7)",   my_mul(0, 7),   0);
    check("mul(7, 0)",   my_mul(7, 0),   0);
    check("mul(3, 4)",   my_mul(3, 4),   12);
    check("mul(-3, 4)",  my_mul(-3, 4),  -12);
    check("mul(3, -4)",  my_mul(3, -4),  -12);
    check("mul(-3, -4)", my_mul(-3, -4), 12);
    check("mul(1, 1)",   my_mul(1, 1),   1);

    printf("\n=== my_div ===\n");
    check("div(12, 3)",   my_div(12, 3),   4);
    check("div(-12, 3)",  my_div(-12, 3),  -4);
    check("div(12, -3)",  my_div(12, -3),  -4);
    check("div(-12, -3)", my_div(-12, -3), 4);
    check("div(7, 2)",    my_div(7, 2),    3);   /* truncates toward zero */
    check("div(0, 5)",    my_div(0, 5),    0);
    check("div(5, 0)",    my_div(5, 0),    0);   /* safe div-by-zero */

    printf("\n=== my_mod ===\n");
    check("mod(10, 3)",   my_mod(10, 3),   1);
    check("mod(9, 3)",    my_mod(9, 3),    0);
    check("mod(-10, 3)",  my_mod(-10, 3),  -1);  /* sign follows a (C99) */
    check("mod(10, -3)",  my_mod(10, -3),  1);
    check("mod(0, 5)",    my_mod(0, 5),    0);
    check("mod(5, 0)",    my_mod(5, 0),    0);   /* safe */

    printf("\n=== my_max ===\n");
    check("max(3, 7)",   my_max(3, 7),   7);
    check("max(7, 3)",   my_max(7, 3),   7);
    check("max(-1, -5)", my_max(-1, -5), -1);
    check("max(4, 4)",   my_max(4, 4),   4);

    printf("\n=== my_min ===\n");
    check("min(3, 7)",   my_min(3, 7),   3);
    check("min(7, 3)",   my_min(7, 3),   3);
    check("min(-1, -5)", my_min(-1, -5), -5);
    check("min(4, 4)",   my_min(4, 4),   4);

    printf("\n=== my_clamp ===\n");
    check("clamp(5,  0, 10)",  my_clamp(5,  0, 10),  5);   /* inside */
    check("clamp(-3, 0, 10)",  my_clamp(-3, 0, 10),  0);   /* below lo */
    check("clamp(15, 0, 10)",  my_clamp(15, 0, 10),  10);  /* above hi */
    check("clamp(0,  0, 10)",  my_clamp(0,  0, 10),  0);   /* on lo */
    check("clamp(10, 0, 10)",  my_clamp(10, 0, 10),  10);  /* on hi */
    check("clamp(-5,-10,-1)",  my_clamp(-5,-10,-1),  -5);  /* all negative */

    printf("\n%d / %d tests passed.\n", g_passed, g_tests);
    return (g_passed == g_tests) ? 0 : 1;
}

#endif /* TEST_MATH */
