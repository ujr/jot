/* Emit HTML5 from Markdown */

/* Notes on HTML5
 * - must start with `<!DOCTYPE html>`, the doctype declaration;
 *   here we do NOT include it, because we expect to be wrapped.
 * - must declare character encoding (using meta charset, or
 *   a BOM, or the HTTP Content-Type header); here we do NOT
 *   declare an encoding, because we emit bytes, and assume
 *   outer/wrapping HTML to do this declaration.
 * - there are many new "semantic" elements, including: section,
 *   article, header, footer, main, aside, nav; but they are not
 *   emitted by this code
 * - some elements were declared obsolete, including: center,
 *   font, tt, strike, frame, applet;
 * - void elements have no end tag and thus no contents; common
 *   examples include: br, hr, img, link, meta; the trailing
 *   slash was required with HTML4, but is no longer with HTML5;
 *   here we do not emit it: `<hr>` instead of `<hr/>`
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "mkdn.h"
#include "pikchr.h"


#define UNUSED(x) ((void)(x))
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')

#define BLOB_ADDLIT(bp, lit) blob_addbuf((bp), "" lit, (sizeof lit)-1)
/* the empty string causes a syntax error if not a literal string;
   and `-1` is necessary because sizeof includes the \0 terminator */

struct html {
  const char *wrapperclass;
};


/** escape < > & as &lt; &gt; &amp; for HTML text */
static void
quote_text(Blob *out, const char *text, size_t size)
{
  size_t i, j = 0;
  while (j < size) {
    for (i = j;
      j < size && text[j] != '<' && text[j] != '>' && text[j] != '&';
      j++);
    blob_addbuf(out, text+i, j-i);
    for (; j < size; j++) {
           if (text[j] == '<') BLOB_ADDLIT(out, "&lt;");
      else if (text[j] == '>') BLOB_ADDLIT(out, "&gt;");
      else if (text[j] == '&') BLOB_ADDLIT(out, "&amp;");
      else break;
    }
  }
}


/** escape < > & ' " as &lt; &gt; &amp; &#39; &quot; for HTML attrs */
static void
quote_attr(Blob *out, const char *text, size_t size)
{
  size_t i, j = 0;
  while (j < size) {
    for (i = j; j < size && text[j] != '<' && text[j] != '>' &&
         text[j] != '&' && text[j] != '"' && text[j] != '\''; j++);
    blob_addbuf(out, text+i, j-i);
    for (; j < size; j++) {
           if (text[j] == '<') BLOB_ADDLIT(out, "&lt;");
      else if (text[j] == '>') BLOB_ADDLIT(out, "&gt;");
      else if (text[j] == '&') BLOB_ADDLIT(out, "&amp;");
      else if (text[j] == '"') BLOB_ADDLIT(out, "&quot;");
      else if (text[j] == '\'') BLOB_ADDLIT(out, "&#39;"); /* HTML5: &apos; */
      else break;
    }
  }
}


static void
html_prolog(Blob *out, void *udata)
{
  struct html *phtml = (struct html *) udata;
  if (phtml && phtml->wrapperclass && phtml->wrapperclass[0]) {
    BLOB_ADDLIT(out, "<div class=\"");
    quote_attr(out, phtml->wrapperclass, strlen(phtml->wrapperclass));
    BLOB_ADDLIT(out, "\">\n");
  }
  else BLOB_ADDLIT(out, "<div class=\"markdown\">\n");
}


static void
html_epilog(Blob *out, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "</div>\n");
}


static void
html_heading(Blob *out, int level, Blob *text, void *udata)
{
  UNUSED(udata);
  if (blob_len(out) > 0) blob_addstr(out, "\n");
  blob_addfmt(out, "<h%d>", level);
  blob_add(out, text);
  blob_addfmt(out, "</h%d>\n", level);
}


static void
html_paragraph(Blob *out, Blob *text, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<p>");
  blob_add(out, text);
  blob_trimend(out);
  BLOB_ADDLIT(out, "</p>\n");
}


static void
html_hrule(Blob *out, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<hr>\n");
}


static void
html_blockquote(Blob *out, Blob *text, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<blockquote>\n");
  blob_add(out, text);
  BLOB_ADDLIT(out, "</blockquote>\n");
}


static void
render_pikchr(Blob *out, const char *info, Blob *text)
{
  static const char *divclass = "pikchr";
  const char *svg;
  int wd, ht, flags = 0;

  UNUSED(info); // TODO get flags from info

  svg = pikchr(blob_str(text), divclass, flags, &wd, &ht);
  if (wd > 0 && ht > 0) {
    BLOB_ADDLIT(out, "<div class=\"");
    quote_attr(out, divclass, strlen(divclass));
    BLOB_ADDLIT(out, "\">\n");
    blob_addstr(out, svg);
    BLOB_ADDLIT(out, "</div>\n");
  }
  else {
    BLOB_ADDLIT(out, "<pre class=\"error\">\n");
    blob_addstr(out, svg); // TODO check: should already be HTML escaped!?
    BLOB_ADDLIT(out, "</pre>\n");
  }
  free((void *) svg);
}


static void
html_codeblock(Blob *out, const char *lang, Blob *text, void *udata)
{
  size_t i, j;
  UNUSED(udata);
  if (!lang) lang = "";
  for (i=0; ISBLANK(lang[i]); i++);
  for (j=i; lang[j] && !ISBLANK(lang[j]); j++);
  if (j > i) {
    if (j-i == 6 && strncmp("pikchr", lang+i, 6) == 0) {
      render_pikchr(out, lang+6, text);
      return;
    }
    BLOB_ADDLIT(out, "<pre><code class=\"language-");
    quote_attr(out, lang+i, j-i);
    BLOB_ADDLIT(out, "\">");
  }
  else {
    BLOB_ADDLIT(out, "<pre><code>");
  }
  blob_add(out, text);
  BLOB_ADDLIT(out, "</code></pre>\n");
}


static void
html_listitem(Blob *out, Blob *text, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<li>");
  blob_add(out, text);
  blob_trimend(out);
  BLOB_ADDLIT(out, "</li>\n");
}


static void
html_list(Blob *out, char type, int start, Blob *text, void *udata)
{
  UNUSED(udata);
  if (type == '.' || type == ')') {
    if (start <= 1) BLOB_ADDLIT(out, "<ol>\n");
    else blob_addfmt(out, "<ol start=\"%d\">\n", start);
    blob_add(out, text);
    BLOB_ADDLIT(out, "</ol>\n");
  }
  else { /* type is '-' or '*' or '+' (but don't check here) */
    BLOB_ADDLIT(out, "<ul>\n");
    blob_add(out, text);
    BLOB_ADDLIT(out, "</ul>\n");
  }
}


static void
html_text(Blob *out, Blob *text, void *udata)
{
  UNUSED(udata);
  quote_text(out, blob_buf(text), blob_len(text));
}


void
mkdnhtml(Blob *out, const char *txt, size_t len, const char *wrap)
{
  struct markdown rndr;
  struct html opts;

  memset(&opts, 0, sizeof(opts));
  memset(&rndr, 0, sizeof(rndr));

  rndr.udata = &opts;

  if (wrap) {
    rndr.prolog = html_prolog;
    rndr.epilog = html_epilog;
    opts.wrapperclass = wrap;
  }

  rndr.heading = html_heading;
  rndr.paragraph = html_paragraph;
  rndr.hrule = html_hrule;
  rndr.blockquote = html_blockquote;
  rndr.codeblock = html_codeblock;
  rndr.listitem = html_listitem;
  rndr.list = html_list;
  rndr.htmlblock = 0; // TODO

  // TODO inline stuff

  rndr.entity = 0;
  rndr.text = html_text;

  markdown(out, txt, len, &rndr);
}
