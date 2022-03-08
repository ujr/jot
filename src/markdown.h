#pragma once
#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <stddef.h>

#include "blob.h"

struct markdown {
  const char *emphchars; /* usually "*_" but may add other ASCII punctuation chars */
  void *udata; /* user data pointer, passed to callbacks */

  /* document callbacks */
  void (*prolog)(Blob *out, void *udata);
  void (*epilog)(Blob *out, void *udata);

  /* block callbacks */
  void (*heading)(Blob *out, int level, Blob *text, void *udata);
  void (*paragraph)(Blob *out, Blob *text, void *udata);
  void (*codeblock)(Blob *out, const char *lang, Blob *text, void *udata);
  void (*blockquote)(Blob *out, Blob *text, void *udata);
  void (*list)(Blob *out, char type, int start, Blob *text, void *udata);
  void (*listitem)(Blob *out, int tightstart, int tightend, Blob *text, void *udata);
  void (*hrule)(Blob *out, void *udata);
  void (*htmlblock)(Blob *out, Blob *text, void *udata);
  /* TOOD table/row/cell */

  /* span callbacks */
  bool (*emphasis)(Blob *out, char c, int n, Blob *text, void *udata);
  bool (*codespan)(Blob *out, Blob *code, void *udata);
  bool (*link)(Blob *out, Blob *link, Blob *title, Blob *body, void *udata);
  bool (*image)(Blob *out, Blob *link, Blob *title, Blob *alt, void *udata);
  bool (*autolink)(Blob *out, char type, const char *text, size_t size, void *udata);
  bool (*htmltag)(Blob *out, const char *text, size_t size, void *udata);
  bool (*linebreak)(Blob *out, void *udata);
  bool (*entity)(Blob *out, const char *text, size_t size, void *udata);

  /* text runs */
  void (*text)(Blob *out, const char *text, size_t size, void *udata);
};


void markdown(Blob *out, const char *txt, size_t len, struct markdown *mkdn);

void mkdnhtml(Blob *out, const char *txt, size_t len, const char *wrap, int pretty);

#endif
