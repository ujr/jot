
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#include "pathbuf.h"


#define INITSIZE 512
#define DIRSEP   '/'


const char *
pathbuf_init(struct pathbuf *pp, const char *prefix)
{
  assert(pp != NULL);
  assert(prefix != NULL);
  size_t len = strlen(prefix);
  size_t size = len + 1; /* room for '\0' */
  if (size < INITSIZE) size = INITSIZE; // TODO else round up to ...?
  pp->path = malloc(size);
  if (!pp->path) return 0; /* no mem */
  memcpy(pp->path, prefix, len+1);
  pp->size = size;
  pp->minlen = pp->len = len;
  return pp->path;
}


const char *
pathbuf_push(struct pathbuf *pp, const char *name)
{
  assert(pp != NULL);
  assert(name != NULL);

  while (*name == DIRSEP) ++name;
  size_t len = strlen(name);
  while (len && name[len-1] == DIRSEP) --len;
  if (pp->len + 1 + len + 1 > pp->size) {
    size_t n = pp->len + 1 + len + 1;
    if (n < 2*pp->size) n = 2*pp->size;
    char *p = realloc(pp->path, n);
    if (!p) return 0;  /* no mem */
    pp->path = p;
    pp->size = n;
  }
  /* append DIRSEP *even* if path ends with DIRSEP: this
     is so that we can overwrite the DIRSEP with \0 in pop */
  pp->path[pp->len++] = DIRSEP;
  memcpy(pp->path + pp->len, name, len);
  pp->len += len;
  pp->path[pp->len] = 0;
  return pp->path;
}


const char *
pathbuf_pop(struct pathbuf *pp)
{
  assert(pp != NULL);
  if (pp->len <= pp->minlen) return pp->path;
  size_t len = pp->len;
  while (len > pp->minlen && pp->path[len-1] != DIRSEP) --len;
  if (len > pp->minlen) --len;
  pp->path[len] = 0;
  pp->len = len;
  return pp->path+len+1;
}


void
pathbuf_free(struct pathbuf *pp)
{
  assert(pp != NULL);
  if (pp->size > 0)
    free(pp->path);
  pp->path = 0;
  pp->len = pp->size = pp->minlen = 0;
}

#ifdef STANDALONE
#include <stdio.h>

int main(int argc, char **argv)
{
  struct pathbuf buf;
  const char *r;

  r = pathbuf_init(&buf, "///");
  printf("init: r=[%s]\n", r);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_push(&buf, ".");
  printf("push: r=[%s] (pushed %s)\n", r, ".");
  r = pathbuf_push(&buf, "foo");
  printf("push: r=[%s] (pushed %s)\n", r, "foo");
  r = pathbuf_push(&buf, "bar");
  printf("push: r=[%s] (pushed %s)\n", r, "bar");
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_push(&buf, "baz");
  printf("push: r=[%s] (pushed %s)\n", r, "baz");
  r = pathbuf_push(&buf, "aar");
  printf("push: r=[%s] (pushed %s)\n", r, "aar");
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  r = pathbuf_pop(&buf);
  printf("pop:  r=[%s] (remains %s)\n", r, buf.path);
  pathbuf_free(&buf);
  return 0;
}
#endif
