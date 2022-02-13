#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

const char *basename(const char *path);
bool streq(const char *s, const char *t);

/* implementations of common functions not in ANSI C */
char *strcopy(const char *s);
size_t strlenmax(const char *s, size_t maxlen);
int strnicmp(const char *s, const char *t, size_t n);

/* <ctype.h> alternatives that ignore locale / assume UTF-8 */
int isSpace(int c);
int isDigit(int c);
int isLower(int c);
int isUpper(int c);
int isAlpha(int c);
int isAlnum(int c);
int toLower(int c);
int toUpper(int c);

#endif
