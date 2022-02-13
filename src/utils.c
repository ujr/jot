
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"


/** pointer to basename portion of given path;
 * assume an implicit separator just before given path;
 * unlike POSIX, the result may be the empty string;
 * special case: basename of "/" is "/"     */
const char *
basename(const char *path)
{
  static const char sep = '/';
  static const char alt = '\\';
  register const char *p;

  if (!path) return 0;

  for (p = path; *p; ) {
    if (*p++ == sep) {
      path = p;  /* for POSIX: if (*p) path = p; */
    }
  }

  for (p = path; *p; ) {
    if (*p++ == alt) {
      path = p;  /* for POSIX: if (*p) path = p; */
    }
  }

  return path;
}


/** convenience around strcmp(3) */
bool
streq(const char *s, const char *t)
{
  if (!s && !t) return true;
  if (!s || !t) return false;
  return strcmp(s, t) == 0;
}


/** strdup(s), which is not in ANSI C */
char *
strcopy(const char *s)
{
  if (!s) return 0;
  size_t l = strlen(s);
  char *t = malloc(l+1);
  if (!t) return 0;
  return memcpy(t, s, l+1);
}


/** strncasecmp(3), which is not in ANSI C */
int
strnicmp(const char *s, const char *t, size_t n)
{
  if (n == 0) return 0;
  n--; /* do last comparison after the loop */
  while (n && *s && *t && (*s == *t || toLower(*s) == toLower(*t))) {
    s++; t++; n--;
  }
  return toLower(*s) - toLower(*t);
}


/** strnlen(s,n), which is not in ANSI C */
size_t
strlenmax(const char *s, size_t maxlen)
{
  const char *p = memchr(s, 0, maxlen);
  return p ? (size_t)(p - s) : maxlen;
}


/*
** Avoid <ctype.h> here as we strictly assume UTF-8 encoded Unicode
** and want to avoid potential locale issues. Tempting to write these
** as macros, but side effects due to multiple evaluation are far too
** likely, so we stick with functions.
*/

int isSpace(int c) { return c==' ' || ('\t'<=c && c<='\r'); }
int isDigit(int c) { return '0'<=c && c<='9'; }
int isLower(int c) { return 'a'<=c && c<='z'; }
int isUpper(int c) { return 'A'<=c && c<='Z'; }
int isAlpha(int c) { return ('a'<=c && c<='z') || ('A'<=c && c<='Z'); }
int isAlnum(int c) { return ('a'<=c && c<='z') || ('A'<=c && c<='Z') || ('0'<=c && c<='9'); }

int toLower(int c) { return isUpper(c) ? c - 'A' + 'a' : c; }
int toUpper(int c) { return isLower(c) ? c - 'a' + 'A' : c; }
