#ifndef MY_MATH_H
#define MY_MATH_H

/*
 * Custom 32-bit integer limits — we are not allowed to include <limits.h>.
 * MY_INT_MIN is written as (-MY_INT_MAX - 1) on purpose: writing the literal
 * -2147483648 directly is parsed as the unary minus of 2147483648, but
 * 2147483648 does not fit in `int`, so the constant would silently widen and
 * trigger -Wimplicit-int-conversion warnings. The (-MAX - 1) idiom is the
 * standard portable workaround.
 */
#define MY_INT_MAX  2147483647
#define MY_INT_MIN  (-MY_INT_MAX - 1)

int my_mul(int a, int b);
int my_div(int a, int b);
int my_mod(int a, int b);
int my_neg(int a);
int my_abs(int a);
int my_clamp(int val, int lo, int hi);
int my_max(int a, int b);
int my_min(int a, int b);

#endif
