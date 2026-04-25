#ifndef MY_STRING_H
#define MY_STRING_H

int   my_strlen(const char *s);
char *my_strcpy(char *dst, const char *src);
int   my_strcmp(const char *a, const char *b);
void  my_itoa(int n, char *buf);
int   my_atoi(const char *s);

void  my_strncpy(char *dst, const char *src, int n);
int   my_strncmp(const char *a, const char *b, int n);

int   my_isdigit(int c);
int   my_isalpha(int c);
int   my_isalnum(int c);

#endif
