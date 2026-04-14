#include "string.h"

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

