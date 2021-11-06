#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"


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
