#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

bool streq(const char *s, const char *t);
char *strcopy(const char *s);
int strnicmp(const char *s, const char *t, size_t n);
const char *basename(const char *path);

int isSpace(int c);
int isDigit(int c);
int isLower(int c);
int isUpper(int c);
int isAlpha(int c);
int isAlnum(int c);
int toLower(int c);
int toUpper(int c);

#endif
