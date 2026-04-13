#include "string.h"

#ifdef TEST_STRING
#include <stdio.h>
#endif

/* --- my_strlen ------------------------------------------------------------
 * Walk s one character at a time until the null terminator is reached.
 * Returns the number of characters before '\0' (not counting it).
 * -------------------------------------------------------------------------- */
int my_strlen(const char *s)
{
    int len;

    len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* --- my_strcpy ------------------------------------------------------------
 * Copy src into dst character by character, including the null terminator.
 * Returns dst so callers can chain the call (mirrors standard strcpy).
 * Caller must ensure dst has enough space — no bounds checking is done.
 * -------------------------------------------------------------------------- */
char *my_strcpy(char *dst, const char *src)
{
    int i;

    i = 0;
    while (src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

/* --- my_strcmp ------------------------------------------------------------
 * Compare two strings character by character.
 * Returns 0 if equal, negative if a < b, positive if a > b.
 * The return value is the difference of the first differing characters,
 * which matches the sign contract of standard strcmp.
 * -------------------------------------------------------------------------- */
int my_strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b)
    {
        a++;
        b++;
    }
    return ((unsigned char)*a - (unsigned char)*b);
}

/* --- my_itoa --------------------------------------------------------------
 * Convert integer n to its decimal string representation in buf.
 * Handles zero, positive, and negative numbers (including INT_MIN).
 *
 * Strategy: accumulate digits in a temporary buffer while keeping n in its
 * original sign — this avoids the INT_MIN overflow that would occur if we
 * tried to negate it.  For negatives each raw digit is negated before
 * storing so it becomes positive ('0' + positive_digit).
 * After the loop the temporary buffer is reversed into buf.
 *
 * buf must be at least 12 bytes wide (sign + 10 digits + '\0').
 * -------------------------------------------------------------------------- */
void my_itoa(int n, char *buf)
{
    char tmp[12]; /* '-' + 10 digits + '\0' covers all 32-bit integers */
    int  i;
    int  j;

    i = 0;

    /* Special case: zero */
    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    if (n < 0)
    {
        /* Work with the negative value directly to avoid INT_MIN overflow.
         * n % 10 is <= 0 for negative n (C99 truncation toward zero),
         * so -(n % 10) gives the correct positive digit value 0-9. */
        while (n != 0)
        {
            tmp[i++] = (char)('0' + (-(n % 10)));
            n /= 10;
        }
        tmp[i++] = '-';
    }
    else
    {
        while (n != 0)
        {
            tmp[i++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }

    /* Reverse tmp into buf */
    j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];
    buf[j] = '\0';
}

/* --- my_atoi --------------------------------------------------------------
 * Convert the leading decimal integer in string s to an int.
 * Skips leading whitespace (space, tab, newline, carriage return).
 * Accepts an optional leading '+' or '-' sign.
 * Stops at the first non-digit character after the sign.
 * Overflow is not guarded — caller should pass well-formed input.
 * -------------------------------------------------------------------------- */
int my_atoi(const char *s)
{
    int result;
    int sign;

    result = 0;
    sign   = 1;

    /* Skip leading whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;

    /* Handle optional sign */
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    /* Accumulate digits */
    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    return result * sign;
}

/* ==========================================================================
 * Self-contained test block — compile with -DTEST_STRING and run the binary.
 *
 *   gcc -Wall -Wextra -DTEST_STRING -Ilibs libs/string.c -o string_test && ./string_test
 *
 * ========================================================================== */
#ifdef TEST_STRING

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"

static int g_tests  = 0;
static int g_passed = 0;

/* Check an integer result */
static void check_int(const char *label, int got, int expected)
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

/* Check a string result — uses my_strcmp so no <string.h> is needed */
static void check_str(const char *label, const char *got, const char *expected)
{
    g_tests++;
    if (my_strcmp(got, expected) == 0)
    {
        printf("[%s] %s  (got \"%s\")\n", PASS, label, got);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got \"%s\", expected \"%s\")\n",
               FAIL, label, got, expected);
    }
}

/* Check the sign of an integer result (negative / zero / positive) */
static void check_sign(const char *label, int got, int expected_sign)
{
    int got_sign;

    g_tests++;
    got_sign = (got > 0) - (got < 0); /* -1, 0, or +1 */
    if (got_sign == expected_sign)
    {
        printf("[%s] %s  (got %d, sign %+d)\n", PASS, label, got, got_sign);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got %d with sign %+d, expected sign %+d)\n",
               FAIL, label, got, got_sign, expected_sign);
    }
}

int main(void)
{
    char buf[64];

    /* ------------------------------------------------------------------ */
    printf("=== my_strlen ===\n");
    check_int("strlen(\"\")",       my_strlen(""),        0);
    check_int("strlen(\"a\")",      my_strlen("a"),       1);
    check_int("strlen(\"hello\")",  my_strlen("hello"),   5);
    check_int("strlen(\"abc\\0x\")",my_strlen("abc"),     3); /* stops at \\0 */

    /* ------------------------------------------------------------------ */
    printf("\n=== my_strcpy ===\n");
    my_strcpy(buf, "");
    check_str("strcpy empty",       buf, "");
    my_strcpy(buf, "hello");
    check_str("strcpy \"hello\"",   buf, "hello");
    my_strcpy(buf, "Tetris!");
    check_str("strcpy \"Tetris!\"", buf, "Tetris!");

    /* ------------------------------------------------------------------ */
    printf("\n=== my_strcmp ===\n");
    check_int("strcmp(\"\",\"\")",        my_strcmp("", ""),           0);
    check_int("strcmp(\"a\",\"a\")",      my_strcmp("a", "a"),         0);
    check_int("strcmp(\"abc\",\"abc\")",  my_strcmp("abc", "abc"),     0);
    check_sign("strcmp(\"a\",\"b\") < 0", my_strcmp("a", "b"),        -1);
    check_sign("strcmp(\"b\",\"a\") > 0", my_strcmp("b", "a"),        +1);
    check_sign("strcmp(\"ab\",\"a\") > 0",my_strcmp("ab", "a"),       +1);
    check_sign("strcmp(\"a\",\"ab\") < 0",my_strcmp("a", "ab"),       -1);

    /* ------------------------------------------------------------------ */
    printf("\n=== my_itoa ===\n");
    my_itoa(0, buf);
    check_str("itoa(0)",            buf, "0");
    my_itoa(1, buf);
    check_str("itoa(1)",            buf, "1");
    my_itoa(-1, buf);
    check_str("itoa(-1)",           buf, "-1");
    my_itoa(1234, buf);
    check_str("itoa(1234)",         buf, "1234");
    my_itoa(-1234, buf);
    check_str("itoa(-1234)",        buf, "-1234");
    my_itoa(2147483647, buf);
    check_str("itoa(INT_MAX)",      buf, "2147483647");
    my_itoa(-2147483648, buf);
    check_str("itoa(INT_MIN)",      buf, "-2147483648");
    my_itoa(100, buf);
    check_str("itoa(100)",          buf, "100");
    my_itoa(-100, buf);
    check_str("itoa(-100)",         buf, "-100");
    my_itoa(10, buf);
    check_str("itoa(10)",           buf, "10");

    /* ------------------------------------------------------------------ */
    printf("\n=== my_atoi ===\n");
    check_int("atoi(\"0\")",          my_atoi("0"),          0);
    check_int("atoi(\"1\")",          my_atoi("1"),          1);
    check_int("atoi(\"-1\")",         my_atoi("-1"),         -1);
    check_int("atoi(\"+1\")",         my_atoi("+1"),         1);
    check_int("atoi(\"1234\")",       my_atoi("1234"),       1234);
    check_int("atoi(\"-1234\")",      my_atoi("-1234"),      -1234);
    check_int("atoi(\" 42\")",        my_atoi("   42"),      42);  /* leading spaces */
    check_int("atoi(\"\\t-7\")",      my_atoi("\t-7"),       -7);  /* leading tab    */
    check_int("atoi(\"12abc\")",      my_atoi("12abc"),      12);  /* stops at 'a'   */
    check_int("atoi(\"\")",           my_atoi(""),           0);   /* empty string   */
    check_int("atoi(\"2147483647\")", my_atoi("2147483647"), 2147483647);

    /* ------------------------------------------------------------------ */
    printf("\n%d / %d tests passed.\n", g_passed, g_tests);
    return (g_passed == g_tests) ? 0 : 1;
}

#endif /* TEST_STRING */
