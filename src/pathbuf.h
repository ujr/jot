#ifndef PATHBUF_H
#define PATHBUF_H

#include <stddef.h>

struct pathbuf {
  char *path;
  size_t len;
  size_t size;
  size_t minlen;
};

const char *pathbuf_init(struct pathbuf *pp, const char *root);
const char *pathbuf_push(struct pathbuf *pp, const char *name);
const char *pathbuf_pop(struct pathbuf *pp);
void pathbuf_free(struct pathbuf *pp);

#endif
