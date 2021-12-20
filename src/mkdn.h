#pragma once
#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <stddef.h>

#include "blob.h"

struct markdown {
  const char *spanchars; // usually * and _, may add ~ + -
  void *udata; // user data pointer, passed to callbacks

  /* document callbacks */
  void (*prolog)(Blob *out, void *udata);
  void (*epilog)(Blob *out, void *udata);
  /* block callbacks */
  void (*heading)(Blob *out, int level, Blob *text, void *udata);
  void (*paragraph)(Blob *out, Blob *text, void *udata);
  void (*codeblock)(Blob *out, const char *lang, Blob *text, void *udata);
  void (*blockquote)(Blob *out, Blob *text, void *udata);
  void (*list)(Blob *out, char type, int start, Blob *text, void *udata);
  void (*listitem)(Blob *out, Blob *text, void *udata);
  void (*hrule)(Blob *out, void *udata);
  void (*htmlblock)(Blob *out, const char *text, void *udata);
  // TOOD table/row/cell
  /* span callbacks */
  bool (*emphasis)(Blob *out, char c, int n, Blob *text, void *udata);
  bool (*codespan)(Blob *out, int n, Blob *text, void *udata);
  bool (*link)(Blob *out, const char *link, const char *title, Blob *body, void *udata);
  bool (*image)(Blob *out, const char *link, const char *title, Blob *alt, void *udata);
  bool (*autolink)(Blob *out, Blob *text, void *udata);
  bool (*linebreak)(Blob *out, void *udata);
  bool (*htmltag)(Blob *out, Blob *text, void *udata);
  /* basics */
  void (*entity)(Blob *out, Blob *name, void *udata);
  void (*text)(Blob *out, Blob *text, void *udata);
};


void markdown(Blob *out, const char *txt, size_t len, struct markdown *mkdn);

void mkdnhtml(Blob *out, const char *txt, size_t len, const char *wrap);

#endif
