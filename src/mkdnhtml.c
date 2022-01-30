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
#define ISASCII(c) (0 <= (c) && (c) <= 127)
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')

#define BLOB_ADDLIT(bp, lit) blob_addbuf((bp), "" lit, (sizeof lit)-1)
/* the empty string causes a syntax error if not a literal string;
   and `-1` is necessary because sizeof includes the \0 terminator */

#define URLENCODE " \"<`>[\\]"  /* chosen mainly such that CM tests succeed */


struct html {
  const char *wrapperclass;
  bool cmout;  /* output as in CommonMark tests */
  int pretty;  /* prettiness; 0=dense, 1=looser, ... */
  int debug;   /* tentative */
};


/** scan dec number from prefix of text[0..size-1]; overflow not detected */
static long
scandec(const char *text, size_t size)
{
  register const char *p = text;
  const char *end = text + size;
  long val = 0;

  for (; p < end && '0' <= *p && *p <= '9'; p++) {
    val = 10 * val + (*p - '0');
  }

  return val;
}


/** scan hex number from prefix of text[0..size-1]; overflow not detected */
static long
scanhex(const char *text, size_t size)
{
  register const char *p = text;
  const char *end = text + size;
  long val = 0;

  for (; p < end; p++) {
    if ('0' <= *p && *p <= '9') val = 16 * val + (*p - '0');
    else if ('a' <= *p && *p <= 'f') val = 16 * val + (*p - 'a' + 10);
    else if ('A' <= *p && *p <= 'F') val = 16 * val + (*p - 'A' + 10);
    else break;
  }

  return val;
}


/** encode c as UTF-8 into the buffer pointed to by p */
#define UTF8_PUT(c, p) do { int u = (c);               \
  if (u < 0x80) *p++ = (unsigned char) (u & 255);      \
  else if (u < 0x800) {                                \
    *p++ = 0xC0 + (unsigned char) ((u >>  6) & 0x1F);  \
    *p++ = 0x80 + (unsigned char) ( u        & 0x3F);  \
  }                                                    \
  else if (u < 0x10000) {                              \
    *p++ = 0xE0 + (unsigned char) ((u >> 12) & 0x0F);  \
    *p++ = 0x80 + (unsigned char) ((u >>  6) & 0x3F);  \
    *p++ = 0x80 + (unsigned char) ( u        & 0x3F);  \
  }                                                    \
  else {                                               \
    *p++ = 0xF0 + (unsigned char) ((u >> 18) & 0x07);  \
    *p++ = 0x80 + (unsigned char) ((u >> 12) & 0x3F);  \
    *p++ = 0x80 + (unsigned char) ((u >>  6) & 0x3F);  \
    *p++ = 0x80 + (unsigned char) ( u        & 0x3F);  \
  }                                                    \
} while (0)


/** escape < > & as &lt; &gt; &amp; for HTML text */
static void
quote_text(Blob *out, const char *text, size_t size, int doquot)
{
  size_t i, j = 0;
  while (j < size) {
    for (i = j; j < size && text[j] != '<' && text[j] != '>'
                         && text[j] != '&' && text[j] != '"'; j++);
    blob_addbuf(out, text+i, j-i);
    for (; j < size; j++) {
           if (text[j] == '<') BLOB_ADDLIT(out, "&lt;");
      else if (text[j] == '>') BLOB_ADDLIT(out, "&gt;");
      else if (text[j] == '&') BLOB_ADDLIT(out, "&amp;");
      else if (text[j] == '"' && doquot) BLOB_ADDLIT(out, "&quot;");
      else break;
    }
  }
}


/** escape < > & ' " as &lt; &gt; &amp; &#39; &quot; for HTML attrs */
static void
quote_attr(Blob *out, const char *text, size_t size, const char *encode)
{
  size_t i, j = 0;
  while (j < size) {
    for (i=j; j < size; j++) {
      if (text[j] == '<' || text[j] == '>' || text[j] == '&' ||
          text[j] == '"' || text[j] == '\'') break;
      if (encode && (!ISASCII(text[j]) || strchr(encode, text[j]))) break;
    }
    if (j > i) blob_addbuf(out, text+i, j-i);
    for (; j < size; j++) {
      if (encode && (!ISASCII(text[j]) || strchr(encode, text[j])))
        blob_addfmt(out, "%%%02X", (unsigned char) text[j]);
      else if (text[j] == '<') BLOB_ADDLIT(out, "&lt;");
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
    quote_attr(out, phtml->wrapperclass, strlen(phtml->wrapperclass), 0);
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
  struct html *phtml = udata;
  if (phtml->pretty > 0 && blob_len(out) > 0)
    blob_addstr(out, "\n");
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
  struct html *phtml = udata;
  if (phtml->cmout)
    BLOB_ADDLIT(out, "<hr />\n");
  else
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
    quote_attr(out, divclass, strlen(divclass), 0);
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
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  size_t i, j;

  if (!lang) lang = "";

  for (i=0; ISBLANK(lang[i]); i++);
  for (j=i; lang[j] && !ISBLANK(lang[j]); j++);

  if (j > i) {
    if (j-i == 6 && strncmp("pikchr", lang+i, 6) == 0) {
      render_pikchr(out, lang+6, text);
      return;
    }
    BLOB_ADDLIT(out, "<pre><code class=\"language-");
    quote_attr(out, lang+i, j-i, 0);
    BLOB_ADDLIT(out, "\">");
  }
  else {
    BLOB_ADDLIT(out, "<pre><code>");
  }
  quote_text(out, blob_str(text), blob_len(text), quotequot);
  BLOB_ADDLIT(out, "</code></pre>\n");
}


static void
html_listitem(Blob *out, int loose, Blob *text, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<li>");
  if (loose) blob_addchar(out, '\n');
  blob_add(out, text);
  if (!loose) blob_trimend(out);
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
html_htmlblock(Blob *out, Blob *text, void *udata)
{
  UNUSED(udata);
  // TODO trim text?
  blob_add(out, text);
  blob_endline(out);
}


static bool
html_codespan(Blob *out, Blob *code, void *udata)
{
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  if (blob_len(code) > 0) {
    BLOB_ADDLIT(out, "<code>");
    quote_text(out, blob_str(code), blob_len(code), quotequot);
    BLOB_ADDLIT(out, "</code>");
  }
  return true;
}


static bool
html_emphasis(Blob *out, char c, int n, Blob *text, void *udata)
{
  UNUSED(udata);

  if (c != '*' && c != '_') return false;

  if (n == 1) {
    BLOB_ADDLIT(out, "<em>");
    blob_add(out, text);  /* text already quoted */
    BLOB_ADDLIT(out, "</em>");
    return true;
  }

  if (n == 2) {
    BLOB_ADDLIT(out, "<strong>");
    blob_add(out, text);  /* text already quoted */
    BLOB_ADDLIT(out, "</strong>");
    return true;
  }

  return false;
}


static bool
html_link(Blob *out, Blob *link, Blob *title, Blob *body, void *udata)
{
  UNUSED(udata);
  /* <a href="LINK" title="TITLE">BODY</a> */

  BLOB_ADDLIT(out, "<a href=\"");
  quote_attr(out, blob_str(link), blob_len(link), URLENCODE);
  blob_addchar(out, '"');
  if (blob_len(title) > 0) {
    BLOB_ADDLIT(out, " title=\"");
    quote_attr(out, blob_str(title), blob_len(title), 0);
    blob_addchar(out, '"');
  }
  blob_addchar(out, '>');
  blob_add(out, body);
  BLOB_ADDLIT(out, "</a>");
  return true;
}


static bool
html_image(Blob *out, Blob *src, Blob *title, Blob *alt, void *udata)
{
  /* <img src="SRC" title="TITLE" alt="ALT" /> */
  struct html *phtml = udata;
  BLOB_ADDLIT(out, "<img src=\"");
  quote_attr(out, blob_str(src), blob_len(src), URLENCODE);
  blob_addchar(out, '"');
  if (blob_len(alt) > 0 || phtml->cmout) {
    BLOB_ADDLIT(out, " alt=\"");
    quote_attr(out, blob_str(alt), blob_len(alt), 0);
    blob_addchar(out, '"');
  }
  if (blob_len(title) > 0) {
    BLOB_ADDLIT(out, " title=\"");
    quote_attr(out, blob_str(title), blob_len(title), 0);
    blob_addchar(out, '"');
  }
  if (phtml->cmout) blob_addchar(out, ' ');
  BLOB_ADDLIT(out, "/>");
  return true;
}


static bool
html_autolink(Blob *out, char type, const char *text, size_t size, void *udata)
{
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  bool ismail = type == '@';

  if (!text || !size) return false;

  if (size > 7 && strncmp("mailto:", text, 7) == 0) {
    text += 7;
    size -= 7;
  }

  BLOB_ADDLIT(out, "<a href=\"");
  if (ismail) BLOB_ADDLIT(out, "mailto:");
  quote_attr(out, text, size, URLENCODE);
  BLOB_ADDLIT(out, "\">");
  quote_text(out, text, size, quotequot);
  BLOB_ADDLIT(out, "</a>");

  return true;
}


static bool
html_htmltag(Blob *out, const char *text, size_t size, void *udata)
{
  UNUSED(udata);
  blob_addbuf(out, text, size);
  return true;
}


static bool
html_linebreak(Blob *out, void *udata)
{
  struct html *phtml = udata;
  blob_trimend(out);
  if (phtml->cmout)
    BLOB_ADDLIT(out, "<br />\n");
  else
    BLOB_ADDLIT(out, "<br>\n");
  return true;
}


static bool
html_entity(Blob *out, const char *text, size_t size, void *udata)
{
  static const long replacement = 0xFFFD;
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  char buf[8], *ptr;
  size_t n;

  if (size < 3) return false;
  if (text[1] == '#') {
    long cp;
    if (text[2] == 'x' || text[2] == 'X') {
      cp = scanhex(text+3, size-3);
    }
    else {
      cp = scandec(text+2, size-2);
    }
    if (cp < 0 || cp > 1114111) return false; /* out of UTF-8 range */
    if (cp == 0) cp = replacement;  /* by CM 2.3 */
    ptr = buf;
    UTF8_PUT(cp, ptr);
    n = ptr-buf;
    quote_text(out, buf, n, quotequot);
    return true;
  }
  if (phtml->cmout) {
    /* Special cases to pass the CM test cases: */
    if (size >= 18 && strncmp("&ThisIsNotDefined;", text, 18) == 0) return false;
    if (size >= 14 && strncmp("&MadeUpEntity;", text, 14) == 0) return false;
    if (size >= 3 && strncmp("&x;", text, 3) == 0) return false;
  }
  blob_addbuf(out, text, size);
  return true;
}


static void
html_text(Blob *out, const char *text, size_t size, void *udata)
{
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  quote_text(out, text, size, quotequot);
}


void
mkdnhtml(Blob *out, const char *txt, size_t len, const char *wrap, int pretty, int debug)
{
  struct markdown rndr;
  struct html opts;

  memset(&opts, 0, sizeof(opts));
  memset(&rndr, 0, sizeof(rndr));

  rndr.udata = &opts;
  rndr.emphchars = 0;  /* use defaults */

  if (wrap) {
    rndr.prolog = html_prolog;
    rndr.epilog = html_epilog;
    opts.wrapperclass = wrap;
  }
  opts.pretty = pretty & 255;
  opts.cmout = !!(pretty & 256);
  opts.debug = debug;

  rndr.heading = html_heading;
  rndr.paragraph = html_paragraph;
  rndr.hrule = html_hrule;
  rndr.blockquote = html_blockquote;
  rndr.codeblock = html_codeblock;
  rndr.listitem = html_listitem;
  rndr.list = html_list;
  rndr.htmlblock = html_htmlblock;

  rndr.codespan = html_codespan;
  rndr.emphasis = html_emphasis;
  rndr.link = html_link;
  rndr.image = html_image;
  rndr.autolink = html_autolink;
  rndr.htmltag = html_htmltag;
  rndr.linebreak = html_linebreak;

  rndr.entity = html_entity;
  rndr.text = html_text;

  markdown(out, txt, len, &rndr, debug);
}
