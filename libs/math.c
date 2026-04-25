/*
 * libs/math.c
 * Author : Rhythm
 * Phase  : 2 (hardened from Phase 1)
 *
 * Custom integer arithmetic — the project is forbidden from using <math.h>.
 * Phase 2 hardening:
 *   - my_neg() added with INT_MIN clamp.
 *   - my_abs() no longer triggers UB on INT_MIN.
 *   - my_div(INT_MIN, -1) returns MY_INT_MAX explicitly (the mathematically
 *     true result 2^31 is not representable).
 *   - my_mod(INT_MIN, -1) returns 0 explicitly (matches the trapped-divide
 *     interpretation: if div clamps, mod is defined to be 0).
 */

#include "math.h"

int my_neg(int a)
{
    /* -INT_MIN overflows. Clamp to MY_INT_MAX as the documented behaviour. */
    if (a == MY_INT_MIN)
        return MY_INT_MAX;
    return -a;
}

int my_abs(int a)
{
    if (a == MY_INT_MIN)
        return MY_INT_MAX;
    return (a < 0) ? -a : a;
}

int my_mul(int a, int b)
{
    int neg;
    int ua;
    int ub;
    int result = 0;

    if (a == 0 || b == 0)
        return 0;

    neg = ((a < 0) != (b < 0));
    ua  = my_abs(a);
    ub  = my_abs(b);

    while (ub > 0) {
        if (ub & 1)
            result += ua;
        ua <<= 1;
        ub >>= 1;
    }

    return neg ? my_neg(result) : result;
}

int my_div(int a, int b)
{
    int neg;
    int ua;
    int ub;
    int quotient;
    int i;

    if (b == 0)
        return 0;

    /* Trap the only signed-overflow division case. */
    if (a == MY_INT_MIN && b == -1)
        return MY_INT_MAX;

    neg = ((a < 0) != (b < 0));
    ua  = my_abs(a);
    ub  = my_abs(b);

    quotient = 0;
    for (i = 31; i >= 0; i--) {
        if ((ua >> i) >= ub) {
            ua       -= (ub << i);
            quotient |= (1U << i);
        }
    }

    return neg ? my_neg(quotient) : quotient;
}

int my_mod(int a, int b)
{
    if (b == 0)
        return 0;
    /* When my_div clamps the INT_MIN/-1 case to MY_INT_MAX the algebraic
     * a - (a/b)*b would not yield 0; short-circuit to keep the contract. */
    if (a == MY_INT_MIN && b == -1)
        return 0;
    return a - my_mul(my_div(a, b), b);
}

int my_max(int a, int b)
{
    return (a > b) ? a : b;
}

int my_min(int a, int b)
{
    return (a < b) ? a : b;
}

int my_clamp(int val, int lo, int hi)
{
    return my_max(lo, my_min(val, hi));
}

#ifdef TEST_MATH
#include <stdio.h>

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, int got, int want)
{
    if (got == want) {
        printf("[PASS] %-28s -> %d\n", name, got);
        g_pass++;
    } else {
        printf("[FAIL] %-28s -> %d (expected %d)\n", name, got, want);
        g_fail++;
    }
}

int main(void)
{
    check("my_div(INT_MIN, -1)",  my_div(MY_INT_MIN, -1),  MY_INT_MAX);
    check("my_mod(INT_MIN, -1)",  my_mod(MY_INT_MIN, -1),  0);
    check("my_neg(INT_MIN)",      my_neg(MY_INT_MIN),      MY_INT_MAX);
    check("my_abs(INT_MIN)",      my_abs(MY_INT_MIN),      MY_INT_MAX);
    check("my_mul(7, -3)",        my_mul(7, -3),           -21);
    check("my_div(-100, 7)",      my_div(-100, 7),         -14);
    check("my_mod(-100, 7)",      my_mod(-100, 7),         -2);

    /* Spot-checks for the regular paths so the hardening did not regress. */
    check("my_mul(0, 99)",        my_mul(0, 99),           0);
    check("my_div(0, 5)",         my_div(0, 5),            0);
    check("my_div(5, 0)",         my_div(5, 0),            0);
    check("my_mod(5, 0)",         my_mod(5, 0),            0);
    check("my_clamp(15, 0, 10)",  my_clamp(15, 0, 10),     10);
    check("my_clamp(-3, 0, 10)",  my_clamp(-3, 0, 10),     0);
    check("my_neg(0)",            my_neg(0),               0);
    check("my_abs(0)",            my_abs(0),               0);

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
#endif
