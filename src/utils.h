#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

bool streq(const char *s, const char *t);
char *strcopy(const char *s);
const char *basename(const char *path);

#endif
