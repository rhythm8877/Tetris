#include "string.h"
#include "math.h"

int my_strlen(const char *s)
{
    int len;

    len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

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

int my_strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b)
    {
        a++;
        b++;
    }
    return ((unsigned char)*a - (unsigned char)*b);
}

void my_itoa(int n, char *buf)
{
    char tmp[12];
    int  i;
    int  j;

    i = 0;

    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    if (n < 0)
    {
        while (n != 0)
        {
            tmp[i++] = (char)('0' + my_neg(my_mod(n, 10)));
            n = my_div(n, 10);
        }
        tmp[i++] = '-';
    }
    else
    {
        while (n != 0)
        {
            tmp[i++] = (char)('0' + my_mod(n, 10));
            n = my_div(n, 10);
        }
    }

    j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];
    buf[j] = '\0';
}

int my_atoi(const char *s)
{
    int result;
    int sign;

    result = 0;
    sign   = 1;

    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;

    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    return result * sign;
}

void my_strncpy(char *dst, const char *src, int n)
{
    int i;

    if (n <= 0)
        return;
    i = 0;
    while (i < n - 1 && src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int my_strncmp(const char *a, const char *b, int n)
{
    int i;

    i = 0;
    while (i < n && a[i] != '\0' && a[i] == b[i])
        i++;
    if (i == n)
        return 0;
    return ((unsigned char)a[i] - (unsigned char)b[i]);
}

int my_isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

int my_isalpha(int c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int my_isalnum(int c)
{
    return (my_isalpha(c) || my_isdigit(c));
}

#ifdef TEST_STRING
#include <stdio.h>

static int g_pass = 0;
static int g_fail = 0;

static void check_int(const char *name, int got, int want)
{
    if (got == want) {
        printf("[PASS] %-40s -> %d\n", name, got);
        g_pass++;
    } else {
        printf("[FAIL] %-40s -> %d (expected %d)\n", name, got, want);
        g_fail++;
    }
}

static void check_str(const char *name, const char *got, const char *want)
{
    if (my_strcmp(got, want) == 0) {
        printf("[PASS] %-40s -> \"%s\"\n", name, got);
        g_pass++;
    } else {
        printf("[FAIL] %-40s -> \"%s\" (expected \"%s\")\n", name, got, want);
        g_fail++;
    }
}

int main(void)
{
    char buf[64];

    /* --- my_itoa --- */
    my_itoa(0, buf);
    check_str("my_itoa(0)", buf, "0");

    my_itoa(12345, buf);
    check_str("my_itoa(12345)", buf, "12345");

    my_itoa(-42, buf);
    check_str("my_itoa(-42)", buf, "-42");

    my_itoa(-1, buf);
    check_str("my_itoa(-1)", buf, "-1");

    my_itoa(MY_INT_MIN, buf);
    check_str("my_itoa(INT_MIN)", buf, "-2147483648");

    my_itoa(MY_INT_MAX, buf);
    check_str("my_itoa(INT_MAX)", buf, "2147483647");

    /* --- my_strncpy: n smaller, equal, larger than strlen --- */
    my_strncpy(buf, "hello", 3);
    check_str("my_strncpy(\"hello\", 3)", buf, "he");

    my_strncpy(buf, "hello", 5);
    check_str("my_strncpy(\"hello\", 5)", buf, "hell");

    my_strncpy(buf, "hello", 6);
    check_str("my_strncpy(\"hello\", 6)", buf, "hello");

    my_strncpy(buf, "hello", 10);
    check_str("my_strncpy(\"hello\", 10)", buf, "hello");

    my_strncpy(buf, "hello", 1);
    check_str("my_strncpy(\"hello\", 1)", buf, "");

    /* --- my_strncmp: n smaller, equal, larger than strlen --- */
    check_int("strncmp(\"abc\",\"abc\",3)",
              my_strncmp("abc", "abc", 3), 0);

    check_int("strncmp(\"abc\",\"abd\",3) < 0",
              my_strncmp("abc", "abd", 3) < 0, 1);

    check_int("strncmp(\"abd\",\"abc\",3) > 0",
              my_strncmp("abd", "abc", 3) > 0, 1);

    check_int("strncmp(\"abc\",\"axyz\",1)",
              my_strncmp("abc", "axyz", 1), 0);

    check_int("strncmp(\"abc\",\"abc\",10)",
              my_strncmp("abc", "abc", 10), 0);

    check_int("strncmp(\"abc\",\"abcd\",10) < 0",
              my_strncmp("abc", "abcd", 10) < 0, 1);

    check_int("strncmp(\"\",\"\",5)",
              my_strncmp("", "", 5), 0);

    /* --- my_isdigit / my_isalpha / my_isalnum --- */
    check_int("my_isdigit('5')", my_isdigit('5') != 0, 1);
    check_int("my_isdigit('0')", my_isdigit('0') != 0, 1);
    check_int("my_isdigit('9')", my_isdigit('9') != 0, 1);
    check_int("my_isdigit('a')", my_isdigit('a'), 0);
    check_int("my_isdigit('/')", my_isdigit('/'), 0);

    check_int("my_isalpha('Z')", my_isalpha('Z') != 0, 1);
    check_int("my_isalpha('a')", my_isalpha('a') != 0, 1);
    check_int("my_isalpha('3')", my_isalpha('3'), 0);
    check_int("my_isalpha(' ')", my_isalpha(' '), 0);

    check_int("my_isalnum('a')", my_isalnum('a') != 0, 1);
    check_int("my_isalnum('7')", my_isalnum('7') != 0, 1);
    check_int("my_isalnum('!')", my_isalnum('!'), 0);

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
#endif
