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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "mkdn.h"
#include "pikchr.h"


#define UNUSED(x) ((void)(x))
#define ISASCII(c) (0 <= (c) && (c) <= 127)
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#define ISALNUM(c) (('0' <= (c) && (c) <= '9') || \
                    ('a' <= (c) && (c) <= 'z') || \
                    ('A' <= (c) && (c) <= 'Z'))

#define NOT_REACHED 0

#define BLOB_ADDLIT(bp, lit) blob_addbuf((bp), "" lit, (sizeof lit)-1)
/* the empty string causes a syntax error if not a literal string;
   and `-1` is necessary because sizeof includes the \0 terminator */

#define URLENCODE " \"<`>[\\]"  /* chosen mainly such that CM tests succeed */


struct html {
  const char *wrapperclass;
  bool cmout;  /* output as in CommonMark tests */
  int pretty;  /* prettiness; 0=dense, 1=looser, ... */
};


static bool _initialized = false;


/* HTML5 entities -- a subset needed to pass the CommonMark tests and
   common German (umlauts) and French (acute, grave, circonflex) stuff.
   See https://html.spec.whatwg.org/entities.json for a complete list.
   You may add entities. Before use, sort the list with entitysort(). */
static struct entity {
  const char *name;
  size_t len;
  unsigned int code1;
  unsigned int code2;
} entities[] = {
  { "quot",                      4,    0x22,    0 },
  { "amp",                       3,    0x26,    0 },
  { "apos",                      4,    0x27,    0 },
  { "lt",                        2,    0x3C,    0 },
  { "gt",                        2,    0x3E,    0 },
  /* Umlauts and German sz ligature: */
  { "Auml",                      4,    0xC4,    0 },
  { "Euml",                      4,    0xCB,    0 },
  { "Iuml",                      4,    0xEF,    0 },
  { "Ouml",                      4,    0xD6,    0 },
  { "Uuml",                      4,    0xDC,    0 },
  { "szlig",                     5,    0xDF,    0 },
  { "auml",                      4,    0xE4,    0 },
  { "euml",                      4,    0xEB,    0 },
  { "iuml",                      4,    0xEF,    0 },
  { "ouml",                      4,    0xF6,    0 },
  { "uuml",                      4,    0xFC,    0 },
  /* Common latin ligatures: */
  { "AElig",                     5,    0xC6,    0 },
  { "aelig",                     5,    0xE6,    0 },
  { "OElig",                     5,  0x0152,    0 },
  { "oelig",                     5,  0x0153,    0 },
  /* More accented latin characters: */
  { "Ccedil",                    6,    0xC7,    0 },
  { "ccedil",                    6,    0xE7,    0 },
  { "Eacute",                    6,    0xC9,    0 },
  { "eacute",                    6,    0xE9,    0 },
  { "acirc",                     5,    0xE2,    0 },
  { "Acirc",                     5,    0xC2,    0 },
  { "ecirc",                     5,    0xEA,    0 },
  { "Ecirc",                     5,    0xCA,    0 },
  { "icirc",                     5,    0xEE,    0 },
  { "Icirc",                     5,    0xCE,    0 },
  { "ocirc",                     5,    0xF4,    0 },
  { "Ocirc",                     5,    0xD4,    0 },
  { "ucirc",                     5,    0xFB,    0 },
  { "Ucirc",                     5,    0xDB,    0 },
  { "agrave",                    6,    0xE0,    0 },
  { "Agrave",                    6,    0xC0,    0 },
  { "egrave",                    6,    0xE8,    0 },
  { "Egrave",                    6,    0xC8,    0 },
  { "igrave",                    6,    0xEC,    0 },
  { "Igrave",                    6,    0xCC,    0 },
  { "ograve",                    6,    0xF2,    0 },
  { "Ograve",                    6,    0xD2,    0 },
  { "ugrave",                    6,    0xF9,    0 },
  { "Ugrave",                    6,    0xD9,    0 },
  /* Spaces, dashes, bullets: */
  { "ndash",                     5,    8211,    0 },
  { "mdash",                     5,    8212,    0 },
  { "nbsp",                      4,     160,    0 },
  { "ensp",                      4,    8194,    0 },
  { "emsp",                      4,    8195,    0 },
  { "shy",                       3,     173,    0 },  /* soft hyphen */
  { "bull",                      4,    8226,    0 },  /* bullet */
  /* Some more to pass the CommonMark tests: */
  { "copy",                      4,     169,    0 },
  { "frac34",                    6,     190,    0 },
  { "Dcaron",                    6,     270,    0 },
  { "HilbertSpace",             12,    8459,    0 },
  { "DifferentialD",            13,    8518,    0 },
  { "ClockwiseContourIntegral", 24,    8754,    0 },
  { "ngE",                       3,    8807,  824 },
  { 0, 0, 0, 0 }
};


static int
entitycmp(const void *a, const void *b)
{
  const struct entity *pa = a;
  const struct entity *pb = b;
  if (pa->len < pb->len) return -1;
  if (pa->len > pb->len) return +1;
  return strncmp(pa->name, pb->name, pa->len);
}


static void
entitysort(void)
{
  size_t msize = sizeof(entities[0]);
  size_t nmemb = sizeof(entities)/msize;
  qsort(entities, nmemb, msize, entitycmp);
}


static struct entity *
entityfind(const char *name, size_t size)
{
  struct entity key;
  size_t msize = sizeof(entities[0]);
  size_t nmemb = sizeof(entities)/msize;
  size_t len = 0;
  while (len < size && ISALNUM(name[len])) len++;
  key.name = name;
  key.len = len;
  return bsearch(&key, entities, nmemb, msize, entitycmp);
}


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


extern size_t scan_entity(const char *text, size_t size);


static size_t
is_entity(const char *text, size_t size)
{
  size_t len = scan_entity(text, size);
  if (!len) return 0;
  struct entity *p = entityfind(text+1, len-1);
  if (!p) return 0;
  return len;
}


/** escape < > & as &lt; &gt; &amp; for HTML text */
static void
quote_text(Blob *out, const char *text, size_t size, int doquot)
{
  size_t i, j, len;
  for (j = 0; j < size; j++) {
    for (i = j; j < size; j++) {
      switch (text[j]) {
        case '<': goto special;
        case '>': goto special;
        case '&':
          len = is_entity(text+j, size-j);
          if (!len) goto special;
          j += len-1;
          break;
        case '"':
          if (doquot) goto special;
          break;
      }
    }
special:
    if (j > i) blob_addbuf(out, text+i, j-i);
    if (j >= size) break;
    switch (text[j]) {
      case '<': BLOB_ADDLIT(out, "&lt;"); break;
      case '>': BLOB_ADDLIT(out, "&gt;"); break;
      case '&': BLOB_ADDLIT(out, "&amp;"); break;
      case '"': BLOB_ADDLIT(out, "&quot;"); break;
      default: assert(NOT_REACHED);
    }
  }
}


/** escape < > &, the latter even if part of a valid entity */
static void
quote_code(Blob *out, const char *text, size_t size, int doquot)
{
  size_t i, j;
  for (j = 0; j < size; j++) {
    for (i = j; j < size; j++) {
      switch (text[j]) {
        case '<': goto special;
        case '>': goto special;
        case '&': goto special;
        case '"':
          if (doquot) goto special;
          break;
      }
    }
special:
    if (j > i) blob_addbuf(out, text+i, j-i);
    if (j >= size) break;
    switch (text[j]) {
      case '<': BLOB_ADDLIT(out, "&lt;"); break;
      case '>': BLOB_ADDLIT(out, "&gt;"); break;
      case '&': BLOB_ADDLIT(out, "&amp;"); break;
      case '"': BLOB_ADDLIT(out, "&quot;"); break;
      default: assert(NOT_REACHED);
    }
  }
}


/** escape < > & ' " as &lt; &gt; &amp; &#39; &quot; for HTML attrs */
static void
quote_attr(Blob *out, const char *text, size_t size, const char *encode)
{
  size_t i, j, len;
  for (j = 0; j < size; j++) {
    for (i = j; j < size; j++) {
      switch (text[j]) {
        case '<': goto special;
        case '>': goto special;
        case '&':
          len = is_entity(text+j, size-j);
          if (!len) goto special;
          j += len-1;
          break;
        case '"': goto special;
        case '\'': goto special;
        default:
          if (encode && (!ISASCII(text[j]) || strchr(encode, text[j])))
            goto special;
      }
    }
special:
    if (j > i) blob_addbuf(out, text+i, j-i);
    if (j >= size) break;
    if (encode && (!ISASCII(text[j]) || strchr(encode, text[j])))
      blob_addfmt(out, "%%%02X", (unsigned char) text[j]);
    else switch (text[j]) {
      case '<': BLOB_ADDLIT(out, "&lt;"); break;
      case '>': BLOB_ADDLIT(out, "&gt;"); break;
      case '&': BLOB_ADDLIT(out, "&amp;"); break;
      case '"': BLOB_ADDLIT(out, "&quot;"); break;
      case '\'': BLOB_ADDLIT(out, "&#39;"); break;  /* HTML5: &apos; */
      default: assert(NOT_REACHED);
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
  blob_endline(out);
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
  quote_code(out, blob_str(text), blob_len(text), quotequot);
  BLOB_ADDLIT(out, "</code></pre>\n");
}


static void
html_listitem(Blob *out, int tightstart, int tightend, Blob *text, void *udata)
{
  UNUSED(udata);
  BLOB_ADDLIT(out, "<li>");
  if (!tightstart) blob_addchar(out, '\n');
  blob_add(out, text);
  if (tightend) blob_trimend(out);
  BLOB_ADDLIT(out, "</li>\n");
}


static void
html_list(Blob *out, char type, int start, Blob *text, void *udata)
{
  UNUSED(udata);
  blob_endline(out);  /* make list start on a new line */
  if (type == '.' || type == ')') {
    if (start == 1 || start < 0) BLOB_ADDLIT(out, "<ol>\n");
    else blob_addfmt(out, "<ol start=\"%d\">\n", start);
    blob_add(out, text);
    BLOB_ADDLIT(out, "</ol>\n");
  }
  else {  /* type is '-' or '*' or '+' (but don't check here) */
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
    quote_code(out, blob_str(code), blob_len(code), quotequot);
    BLOB_ADDLIT(out, "</code>");
  }
  return true;
}


static bool
html_emphasis(Blob *out, char c, int n, Blob *text, void *udata)
{
  UNUSED(udata);

  if (c != '*' && c != '_') return false;
  // TODO could support others like ~~foo~~ for <del>foo</del>

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
  struct entity *p;
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  char buf[16], *ptr;
  size_t n;

  if (size < 3 && text[0] != '&') return false;
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

  /* Named character reference: */
  p = entityfind(text+1, size-1);
  if (p && p->code1) {
    ptr = buf;
    UTF8_PUT(p->code1, ptr);
    if (p->code2) {
      UTF8_PUT(p->code2, ptr);
    }
    n = ptr-buf;
    quote_text(out, buf, n, quotequot);
    return true;
  }

  return false;
}


static void
html_text(Blob *out, const char *text, size_t size, void *udata)
{
  struct html *phtml = udata;
  int quotequot = phtml->cmout;
  quote_text(out, text, size, quotequot);
}


void
mkdnhtml(Blob *out, const char *txt, size_t len, const char *wrap, int pretty)
{
  struct markdown rndr;
  struct html opts;

  if (!_initialized) {
    entitysort();
    _initialized = true;
  }

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

  markdown(out, txt, len, &rndr);
}
