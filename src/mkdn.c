
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "log.h"
#include "mkdn.h"
#include "utils.h"


/* Markdown has block elements and inline (span) elements.
 * Reference style links (and images) are forward-looking,
 * so we need to collect link definitions before the actual
 * rendering i a second pass over the Markdown. The second
 * pass will parse the input into a sequence of "blocks"
 * and, within each block, parse "inlines" such as emphasis
 * or links. Some block types can contain other blocks.
 *
 * The parser here is inspired by the one in Fossil SCM,
 * which itself derives from the parser by Natacha Porté.
 */


#define ISCNTRL(c) ((c) < 32 || (c) == 127)
#define ISPUNCT(c) (('!' <= (c) && (c) <= '/')  /* 21..2F */ \
                 || (':' <= (c) && (c) <= '@')  /* 3A..40 */ \
                 || ('[' <= (c) && (c) <= '`')  /* 5B..60 */ \
                 || ('{' <= (c) && (c) <= '~')) /* 7B..7E */
// ! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~
#define ISSPACE(c) ((c) == ' ' || ('\t' <= (c) && (c) <= '\r'))
// the above includes VT and FF, which we may not want?!
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#define ISDIGIT(c) ('0' <= (c) && (c) <= '9')
#define ISXDIGIT(c) (ISDIGIT(c) || ('a' <= (c) && (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))
#define ISLOWER(c) ('a' <= (c) && (c) <= 'z')
#define ISUPPER(c) ('A' <= (c) && (c) <= 'Z')
#define ISALPHA(c) (ISLOWER(c) || ISUPPER(c))
#define ISALNUM(c) (ISALPHA(c) || ISDIGIT(c))
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

#define UNUSED(x) ((void)(x))
#define OUT_OF_MEMORY 0
#define INVALID_HTMLBLOCK_KIND 0
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/** length of array (number of items) */
#define ARLEN(a) (sizeof(a)/sizeof((a)[0]))


typedef struct parser Parser;

typedef size_t (*SpanProc)(
  Blob *out, const char *text, size_t size,
  size_t prefix, Parser *parser);

struct parser {
  struct markdown render;
  void *udata;
  int nesting_depth;
  int in_link;               /* to inhibit links within links */
  Blob linkdefs;             /* collected link definitions */
  Blob *blob_pool[32];
  size_t pool_index;
  SpanProc active_chars[128];
  const struct tagname *pretag;
  const struct tagname *scripttag;
  const struct tagname *styletag;
  const struct tagname *textareatag;
};


typedef struct slice {
  const char *s;
  size_t n;
} Slice;


static Slice slice(const char *s, size_t len) {
  Slice slice = { s, len };
  return slice;
}


typedef struct linkdef {
  Slice id;
  Slice link;
  Slice title;
} Linkdef;


/** html block tags, ordered by length, then lexicographically */
static const struct tagname {
  const char *name;
  size_t len;
} tagnames[] = {
  { "p",           1 },
  { "dd",          2 },
  { "dl",          2 },
  { "dt",          2 },
  { "h1",          2 },
  { "h2",          2 },
  { "h3",          2 },
  { "h4",          2 },
  { "h5",          2 },
  { "h6",          2 },
  { "hr",          2 },
  { "li",          2 },
  { "ol",          2 },
  { "td",          2 },
  { "th",          2 },
  { "tr",          2 },
  { "ul",          2 },
  { "col",         3 },
  { "dir",         3 },
  { "div",         3 },
  { "nav",         3 },
  { "pre",         3 },
  { "base",        4 },
  { "body",        4 },
  { "form",        4 },
  { "head",        4 },
  { "html",        4 },
  { "link",        4 },
  { "main",        4 },
  { "menu",        4 },
  { "aside",       5 },
  { "frame",       5 },
  { "param",       5 },
  { "style",       5 },
  { "table",       5 },
  { "tbody",       5 },
  { "tfoot",       5 },
  { "thead",       5 },
  { "title",       5 },
  { "track",       5 },
  { "center",      6 },
  { "dialog",      6 },
  { "figure",      6 },
  { "footer",      6 },
  { "header",      6 },
  { "iframe",      6 },
  { "legend",      6 },
  { "option",      6 },
  { "script",      6 },
  { "source",      6 },
  { "address",     7 },
  { "article",     7 },
  { "caption",     7 },
  { "details",     7 },
  { "section",     7 },
  { "summary",     7 },
  { "basefont",    8 },
  { "colgroup",    8 },
  { "fieldset",    8 },
  { "frameset",    8 },
  { "menuitem",    8 },
  { "noframes",    8 },
  { "optgroup",    8 },
  { "textarea",    8 },
  { "blockquote", 10 },
  { "figcaption", 10 },
};


static int
tagname_cmp(const void *a, const void *b)
{
  const struct tagname *pa = a;
  const struct tagname *pb = b;
  if (pa->len < pb->len) return -1;
  if (pa->len > pb->len) return +1;
  return strnicmp(pa->name, pb->name, pa->len);
}


static const struct tagname *
tagname_find(const char *text, size_t size)
{
  struct tagname key;
  size_t nmemb = ARLEN(tagnames);
  size_t msize = sizeof(tagnames[0]);
  size_t j = 0;

  /* TOOD /[A-Za-z][-A-Za-z0-9]star/ would be correcter */
  while (j < size && ISALPHA(text[j])) j++;
  if (j >= size) return 0;

  key.name = text;
  key.len = j;
  return bsearch(&key, tagnames, nmemb, msize, tagname_cmp);
}


/** return length of HTML tag at text or 0 if malformed */
static size_t
taglength(const char *text, size_t size)
{
  size_t len = 0;
  /* <x> is shortest possible tag */
  if (size < 3) return 0;
  if (text[len] != '<') return 0;
  if (text[++len] == '/') len++;
  if (!ISALPHA(text[len])) return 0;
  while (len < size && (ISALNUM(text[len]) || text[len] == '-')) len++;
  if (len >= size) return 0;
  int c = text[len];
  if (c == '/' && len+1 < size && text[len+1] == '>') return len+2;
  if (c != '>' && !ISSPACE(c)) return 0;
  /* scan over (possible quoted) attributes: */
  int quote = 0;
  while (len < size && (c=text[len]) != 0 && (c != '>' || quote)) {
    if (c == quote) quote = 0;
    else if (!quote && (c == '"' || c == '\'')) quote = c;
    len++;
  }
  if (len >= size || text[len] != '>') return 0;
  return len+1;
}


static int
linkdef_cmp(const void *a, const void *b)
{
  const struct linkdef *pa = a;
  const struct linkdef *pb = b;
  if (pa->id.n < pb->id.n) return -1;
  if (pa->id.n > pb->id.n) return +1;
  return strnicmp(pa->id.s, pb->id.s, pa->id.n);
}


/** find link by its label; return true iff found */
static int
linkdef_find(
  const char *text, size_t size, Parser *parser,
  struct slice *link, struct slice *title)
{
  struct linkdef key, *ptr;
  size_t msize = sizeof(struct linkdef);
  size_t nmemb = blob_len(&parser->linkdefs)/msize;
  if (!nmemb) return 0;
  key.id = slice(text, size);
  key.link = slice(0, 0);
  key.title = slice(0, 0);
  ptr = bsearch(&key, blob_buf(&parser->linkdefs), nmemb, msize, linkdef_cmp);
  if (!ptr) return 0;
  if (link) *link = ptr->link;
  if (title) *title = ptr->title;
  return 1;
}


/** scan autolink; return length and type: link / mail / mailto */
static size_t
scan_autolink(const char *text, size_t size, char *ptype)
{
  /* by CM 6.5 an autolink is either an absolute URI or a mail addr in
     angle brackets; absolute URI has scheme then ':' then anything but
     space nor < nor > nor cntrl; scheme is [A-Za-z][A-Za-z0-9+.-]{1,31};
     mail addr is defined by a monstrous regex, which we oversimplify here;
     backslashes are NOT expanded in autolinks; note that raw HTML tags
     are distinguished by NOT having a : nor @ in tag name */

  static const char *extra = "@.-+_=.~"; /* allowed in mail addr beyond alnum */
  size_t j, max, nat;
  bool mailto = false;

  if (ptype) *ptype = '?';
  if (size < 4) return 0;
  if (text[0] != '<') return 0;

  j = 1;
  if (!ISALPHA(text[j])) goto trymail;
  max = 33; if (size < max) max = size;
  for (++j; j < max && (ISALNUM(text[j]) || text[j] == '+'
                     || text[j] == '.' || text[j] == '-'); j++);
  if (2 < j && j < max && text[j] == ':') j++;
  else goto trymail;
  if (strncmp(text+1, "mailto:", 7) == 0) {
    mailto = true;
    goto trymail;
  }
  for (; j < size && text[j] != '>'; j++) {
    if (ISSPACE(text[j]) || text[j] == '<' || ISCNTRL(text[j])) return 0;
  }
  if (j >= size || text[j] != '>') return 0;
  if (ptype) *ptype = 'L'; /* hyperlink */
  return j+1;

trymail:
  // simplistic: scan over "reasonable" characters until '>'; mail if exactly one '@':
  for (nat = 0; j < size && (ISALNUM(text[j]) || strchr(extra, text[j])); j++) {
    if (text[j] == '@') nat++;
  }
  if (j >= size || text[j] != '>' || nat != 1) return 0;
  if (ptype) *ptype = mailto ? 'M' : '@';
  return j+1;
}


/** scan link label like [foo]; return length or zero */
static size_t
scan_label(const char *text, size_t size)
{
  size_t j = 0;
  if (j >= size || text[j] != '[') return 0;
  for (++j; j < size; j++) {
    if (text[j] == ']') break;
    if (text[j] == '\\') j++;
  }
  if (j >= size || text[j] != ']') return 0;
  return j+1;
}


/** scan link url and title; return length or 0 on failure */
static size_t
scan_link_and_title(const char *text, size_t size,
  struct slice *link, struct slice *title)
{
  size_t linkofs, linkend;
  size_t titlofs, titlend;
  size_t j = 0, end;

  if (j < size && text[j] == '<') {
    /* link in angle brackets, no line endings, no unescaped < or > */
    linkofs = ++j;
    for (; j < size && text[j] != '>'; j++) {
      if (text[j] == '\\') { j++; continue; }
      if (text[j] == '\n' || text[j] == '\r') return 0;
    }
    if (j >= size || text[j] != '>') return 0;
    linkend = j++;
  }
  else {
    /* plain link, no cntrl, no space, parens only if escaped or balanced */
    int level = 1;
    linkofs = j;
    for (; j < size && text[j] != ' ' && !ISCNTRL(text[j]); j++) {
      if (text[j] == '\\') { j++; continue; }
      else if (text[j] == '(') level++;
      else if (text[j] == ')')
        if (--level <= 0) break;
    }
    linkend = j;
  }
  end = j;

  /* skip \s*\n?\s*, that is, white space but at most one newline: */
  while (j < size && ISBLANK(text[j])) j++;
  if (j < size && (text[j] == '\n' || text[j] == '\r')) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  while (j < size && ISBLANK(text[j])) j++;

  /* optional title, may contain newlines, but no blank line: */
  titlofs = titlend = 0;
  if (j < size && (text[j] == '"' || text[j] == '\'' || text[j] == '(')) {
    char delim = text[j++], blank = 0;
    if (delim == '(') delim = ')';
    titlofs = j;
    for (; j < size && text[j] != delim; j++) {
      if (text[j] == '\\') { j++; continue; }
      if (text[j] == '\n' || text[j] == '\r') {
        if (blank) return 0; /* empty line in title not allowed */
        blank = 1;
      }
      else if (!ISBLANK(text[j])) blank = 0;
    }
    titlend = j;
    if (titlend < titlofs || text[j] != delim) return 0;
    end = ++j; /* consume the closing delim */
  }

  if (link) *link = slice(text+linkofs, linkend-linkofs);
  if (title) *title = slice(text+titlofs, titlend-titlofs);

  return end;
}


static bool
too_deep(Parser *parser)
{
  return parser->nesting_depth > 100;
}


static Blob*
pool_get(Parser *parser)
{
  static const Blob empty = BLOB_INIT;
  Blob *ptr;
  if (parser->pool_index > 0)
    return parser->blob_pool[--parser->pool_index];
  ptr = malloc(sizeof(*ptr));
  if (!ptr) {
    perror("malloc");
    abort(); // TODO error handler
  }
  *ptr = empty;
  return ptr;
}


static void
pool_put(Parser *parser, Blob *blob)
{
  if (!blob) return;
  if (parser->pool_index < ARLEN(parser->blob_pool)) {
    blob_clear(blob);
    parser->blob_pool[parser->pool_index++] = blob;
  }
  else {
    log_debug("mkdn: blob pool exhausted; freeing blob instead of caching");
    blob_free(blob);
    free(blob);
  }
}


static void
parse_inlines(Blob *out, const char *text, size_t size, Parser *parser)
{
  SpanProc action;
  size_t i, j, n;
  /* copy chars to output, looking for "active" chars */
  i = j = 0;
  while (j < size) {
    for (; j < size; j++) {
      unsigned uc = ((unsigned) text[j]) & 0x7F;
      action = parser->active_chars[uc];
      if (action) break;
    }
    if (j > i) {
      if (parser->render.text)
        parser->render.text(out, text+i, j-i, parser->udata);
      else blob_addbuf(out, text+i, j-i);
      i = j;
    }
    if (j >= size) break;
    assert(action != 0);
    n = action(out, text+j, size-j, j, parser);
    if (n) { j += n; i = j; } else j += 1;
  }
}


static size_t
do_escape(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  UNUSED(prefix);

  /* by CM 2.4, backslash escapes only punctuation, otherwise literal */
  if (size > 1 && ISPUNCT(text[1])) {
    if (parser->render.text)
      parser->render.text(out, text+1, 1, parser->udata);
    else blob_addbuf(out, text+1, 1);
    return 2;
  }

  return 0;
}


static void
do_escapes(Blob *out, const char *text, size_t size)
{
  size_t i, j;
  for (i = j = 0; j < size; ) {
    for (; j < size; j++) {
      if (text[j] == '\\') break;
    }
    if (j > i) {
      blob_addbuf(out, text+i, j-i);
      i = j;
    }
    if (j >= size) break;
    assert(i == j);
    assert(text[j] == '\\');
    j += 1; /* leave i unchanged to add \ to run of text */
    if (j < size && ISPUNCT(text[j])) {
      blob_addchar(out, text[j]);
      i = j += 1;
    }
  }
}


static size_t
do_entity(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  /* by CM 2.5, only valid HTML5 entity names must be parsed; here we
     only parse for syntactical correctness and let the callback decide: */
  size_t i, j = 0;
  UNUSED(prefix);
  assert(j < size && text[j] == '&');
  if (++j < size && text[j] == '#') {
    if (++j < size && (text[j] == 'x' || text[j] == 'X')) {
      j++;
      for (i=j; j < size && ISXDIGIT(text[j]); j++);
      if (j <= i) return 0;  /* need at least one hex digit */
      if (j-i > 6) return 0; /* but at most 7 hex digits */
    }
    else {
      for (i=j; j < size && ISDIGIT(text[j]); j++);
      if (j <= i) return 0;  /* need at least one digit */
      if (j-i > 7) return 0; /* but at most 7 digits */
    }
  }
  else {
    for (i=j; j < size && ISALNUM(text[j]); j++);
    if (j <= i) return 0;  /* need at least one char */
  }
  if (j < size && text[j] == ';') j++;
  else return 0;
  assert(parser->render.entity != 0);
  return parser->render.entity(out, text, j, parser->udata) ? j : 0;
}


static size_t
do_linebreak(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  size_t extra = (text[0] == '\r' && size > 1 && text[1] == '\n') ? 1 : 0;

  /* look back, if possible, for two blanks */
  if (prefix >= 2 && text[-1] == ' ' && text[-2] == ' ') {
    blob_trimend(out); blob_addchar(out, ' ');
    assert(parser->render.linebreak != 0);
    return parser->render.linebreak(out, parser->udata) ? 1 + extra : 0;
  }

  /* NB. backslash newline is NOT handled by do_escape */
  if (prefix >= 1 && text[-1] == '\\') {
    blob_trunc(out, blob_len(out)-1); blob_addchar(out, ' ');
    assert(parser->render.linebreak != 0);
    return parser->render.linebreak(out, parser->udata) ? 1 + extra : 0;
  }

  /* trim trailing space and preserve "soft" breaks (CM 6.8): */
  blob_trimend(out);
  blob_addchar(out, '\n');
  return 1 + extra;
}


static size_t
do_codespan(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  size_t i, j, end, oticks, cticks;
  char delim;
  UNUSED(prefix);

  if (size < 2) return 0;
  delim = text[0];
  for (j = 1; j < size && text[j] == delim; j++);
  oticks = j;

  for (cticks = 0; j < size && cticks < oticks; j++) {
    if (text[j] == delim) cticks += 1;
    else cticks = 0;
  }

  if (cticks < oticks) return 0;

  i = oticks; end = j; j -= cticks;

  /* begins and ends with blank? trim one blank: */
  if (i+2 < j && text[i] == ' ' && text[j-1] == ' ') {
    i++; j--;
  }

  assert(parser->render.codespan != 0);
  return parser->render.codespan(out, text+i, j-i, parser) ? end : 0;
}


static size_t
do_anglebrack(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  /* autolink -or- raw html tag; see CM 6.5 and 6.6 */
  char type;
  size_t len;
  UNUSED(prefix);
  len = taglength(text, size);
  if (len) {
    if (parser->render.htmltag)
      parser->render.htmltag(out, text, len, parser->udata);
    return len;
  }
  len = scan_autolink(text, size, &type);
  if (len) {
    if (parser->render.autolink)
      parser->render.autolink(out, type, text+1, len-2, parser->udata);
    return len;
  }
  return 0;
}


// TODO hook img to ! and look forward (not [ and backwards)
static size_t
do_link(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  /* Types of links:
      1. inline:    [text](dest "title")
      2. reference: [text][ref]
      3. collapsed: [text][]     let ref = text
      4. shortcut:  [text]       let ref = text */
  size_t j = 0, len, textend;
  struct slice linkslice, titleslice;
  Blob *body, *link, *title;
  int img, level, ok = 0;

  if (parser->in_link > 0) return 0;
  img = prefix > 0 && text[-1] == '!' && (prefix < 2 || text[-2] != '\\');
  if (img && !parser->render.image) return 0;
  if (!img && !parser->render.link) return 0;

  /* look for closing bracket: */
  for (level = 1, ++j; j < size; j++) {
    if (text[j] == '\\') { j++; continue; }
    if (text[j] == '[') level += 1;
    else if (text[j] == ']') {
      if (--level <= 0) break;
    }
  }
  if (j >= size) return 0;  /* unmatched '[' */
  textend = j++;  /* eat closing bracket */

  /* inline or reference link must follow immediately: */
  if (j < size && text[j] == '(') {
    /* inline link */
    for (++j; j < size && ISBLANK(text[j]); j++);
    len = scan_link_and_title(text+j, size-j, &linkslice, &titleslice);
    if (!len && text[j] != ')') return 0; /* allow empty () */
    j += len;
    while (j < size && ISBLANK(text[j])) j++;
    if (j >= size || text[j] != ')') return 0;
    j++; /* eat closing paren */
  }
  else if (j < size && text[j] == '[') {
    len = scan_label(text+j, MIN(size-j, 999));
    if (!len) return 0;
    // TODO process escapes (?) and trim
    if (len > 2) { /* not [] */
      if (!linkdef_find(text+j+1, len-2, parser, &linkslice, &titleslice))
        return 0;
    }
    else {
      if (!linkdef_find(text+1, textend-1, parser, &linkslice, &titleslice))
        return 0;
    }
    j += len; /* consume the ']' */
  }
  else {
    /* shortcut link */
    if (!linkdef_find(text+1, textend-1, parser, &linkslice, &titleslice))
      return 0;
  }

  body = pool_get(parser);
  link = pool_get(parser);
  title = pool_get(parser);

  /* parse body inlines only for link, not for image: */
  if (img) blob_addbuf(body, text+1, textend-1);
  else {
    parser->in_link++;
    parse_inlines(body, text+1, textend-1, parser);
    parser->in_link--;
  }

  /* process escapes: */
  do_escapes(link, linkslice.s, linkslice.n);
  do_escapes(title, titleslice.s, titleslice.n);

  if (img) {
    if (blob_len(out) > 0 && blob_byte(out, blob_len(out)-1))
      blob_trunc(out, blob_len(out)-1);
    assert(parser->render.image != 0);
    ok = parser->render.image(out, link, title, body, parser->udata);
  }
  else {
    assert(parser->render.link != 0);
    ok = parser->render.link(out, link, title, body, parser->udata);
  }

  pool_put(parser, title);
  pool_put(parser, link);
  pool_put(parser, body);
  return ok ? j : 0;
}

// TODO other span processors


static void
parse_block(Blob *out, const char *text, size_t size, Parser *parser);


/** scan past next newline; return #bytes scanned */
static size_t
scanline(const char *text, size_t size)
{
  size_t i = 0;
  while (i < size && text[i] != '\n' && text[i] != '\r') i++;
  if (i < size) i++;
  if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
  return i;
}


static size_t
skipcode(const char *text, size_t size)
{
  size_t i, oticks, cticks;
  char delim;

  if (size < 2) return 0;
  delim = text[0];
  for (i = 1; i < size && text[i] == delim; i++);
  oticks = i;

  for (cticks = 0; i < size && cticks < oticks; i++) {
    if (text[i] == delim) cticks += 1;
    else cticks = 0;
  }

  // past closing ticks or past opening ticks (if not closed)
  return cticks == oticks ? i : oticks;
}


/** skip up to three initial blanks */
static size_t
preblanks(const char *text, size_t size)
{
  size_t i = 0;
  while (i < 4 && i < size && text[i] == ' ') i += 1;
  return i < 4 ? i : 0;
}


/** check for a blank line; return line length or 0 */
static size_t
is_blankline(const char *text, size_t size)
{
  size_t i;
  for (i=0; i < size && text[i] != '\n' && text[i] != '\r'; i++) {
    if (!ISBLANK(text[i])) return 0;
  }
  if (i < size) i++;
  if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
  return i;
}


/** link definition: [label]: link "title" */
static size_t
is_linkdef(const char *text, size_t size, Linkdef *pdef)
{
  size_t idofs, idend;
  struct slice link, title;
  size_t len, j = preblanks(text, size);

  len = scan_label(text+j, size-j);
  if (!len) return 0;
  idofs = j+1;
  j += len;
  idend = j-1;

  /* colon must immediately follow the bracket */
  if (j >= size || text[j] != ':') return 0;
  // any amount of optional space, but at most one newline
  for (++j; j < size && ISBLANK(text[j]); j++);
  if (j >= size) return 0;
  if (text[j] == '\n' || text[j] == '\r') j++;
  if (j >= size) return 0;
  if (text[j] == '\n' && text[j-1] == '\r') j++;
  while (j < size && ISBLANK(text[j])) j++;

  len = scan_link_and_title(text+j, size-j, &link, &title);
  if (!len) return 0;
  else j += len;
  len = is_blankline(text+j, size-j);
  if (!len) return 0; /* junk after link def */
  else j += len;

  if (pdef) {
    pdef->id = slice(text+idofs, idend-idofs);
    pdef->link = link;
    pdef->title = title;
  }

  return j;
}


/** check for a setext underlining; return line length or 0 */
static size_t
is_setextline(const char *text, size_t size, int *plevel)
{
  size_t len, i = preblanks(text, size);
  if (plevel) *plevel = 0;
  if (i < size && text[i] == '=') {
    for (++i; i < size && text[i] == '='; i++);
    len = is_blankline(text+i, size-i);
    if (!len) return 0;
    if (plevel) *plevel = 1;
    return i+len;
  }
  if (i < size && text[i] == '-') {
    for (++i; i < size && text[i] == '-'; i++);
    len = is_blankline(text+i, size-i);
    if (!len) return 0;
    if (plevel) *plevel = 2;
    return i+len;
  }
  return 0;
}


/** check for an atx heading prefix; return prefix length or 0 */
static size_t
is_atxline(const char *text, size_t size, int *plevel)
{
  int level;
  size_t i = preblanks(text, size);
  for (level = 0; i < size && text[i] == '#' && level < 7; i++, level++);
  if (!(1 <= level && level <= 6)) return 0;
  /* an empty heading is allowed: */
  if (i >= size || text[i] == '\n' || text[i] == '\r') {
    if (plevel) *plevel = level;
    return i;
  }
  /* otherwise need at least one blank or tab: */
  if (!ISBLANK(text[i])) return 0;
  for (++i; i < size && ISBLANK(text[i]); i++);
  if (plevel) *plevel = level;
  return i;
}


/** parse atx heading; return #chars scanned if found, else 0 */
static size_t
parse_atxheading(Blob *out, const char *text, size_t size, Parser *parser)
{
  int level;
  size_t start, end, len;
  size_t i = is_atxline(text, size, &level);
  if (!i) return 0;
  start = i;
  i += scanline(text+i, size-i);
  len = i;
  /* scan back over blanks, hashes, and blanks again: */
  while (i > start && ISSPACE(text[i-1])) i--;
  end = i;
  while (i > start && text[i-1] == '#') i--;
  if (i > start && ISBLANK(text[i-1])) {
    for (--i; i > start && ISBLANK(text[i-1]); i--);
    end = i;
  }
  else if (i == start) end = i;

  if (parser->render.heading) {
    Blob *title = pool_get(parser);
    parse_inlines(title, text+start, end-start, parser);
    parser->render.heading(out, level, title, parser->udata);
    pool_put(parser, title);
  }
  return len;
}


/** check for codeblock prefix; return prefix length or 0 */
static size_t
is_codeline(const char *text, size_t size)
{
  if (size > 0 && text[0] == '\t') return 1;
  if (size > 3 && text[0] == ' ' && text[1] == ' ' &&
                  text[2] == ' ' && text[3] == ' ') return 4;
  return 0;
}


static size_t
parse_codeblock(Blob *out, const char *text, size_t size, Parser *parser)
{
  static const char *nolang = "";
  size_t i, i0, len, blankrun = 0;
  Blob *temp = pool_get(parser);

  for (i = 0; i < size; ) {
    len = is_codeline(text+i, size-i);
    if (len) { i += len;
      if (blankrun) blob_addchar(temp, '\n');
      blankrun = 0;
    }
    else {
      len = is_blankline(text+i, size-i);
      if (!len) break; /* not indented, not blank: end code block */
      i += len;
      blankrun += 1;
      continue;
    }
    i0 = i;
    while (i < size && text[i] != '\n' && text[i] != '\r') i++;
    if (i > i0) blob_addbuf(temp, text+i0, i-i0);
    if (i < size) i++;
    if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
    blob_addchar(temp, '\n');
  }

  if (parser->render.codeblock)
    parser->render.codeblock(out, nolang, temp, parser->udata);
  pool_put(parser, temp);
  return i;
}


/** check for blockquote prefix; return prefix length or 0 */
static size_t
is_quoteline(const char *text, size_t size)
{
  size_t i = preblanks(text, size);
  if (i >= size || text[i] != '>') return 0;
  i += 1;
  if (i < size && ISBLANK(text[i])) i += 1;
  return i;
}


static size_t
parse_blockquote(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t i, i0, len;
  Blob *temp = pool_get(parser);

  for (i = 0; i < size; ) {
    len = is_quoteline(text+i, size-i);
    if (len) { i += len; }
    else {
      len = is_blankline(text+i, size-i);
      if (len && (i+len >= size ||
                  (!is_quoteline(text+i+len, size-i-len) &&
                   !is_blankline(text+i+len, size-i-len))))
        break; /* empty line followed by non-quote line ends quote */
    }
    i0 = i;
    while (i < size && text[i] != '\n' && text[i] != '\n') i++;
    if (i > i0) blob_addbuf(temp, text+i0, i-i0);
    if (i < size) i++;
    if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
    blob_addchar(temp, '\n');
  }

  if (!too_deep(parser)) {
    Blob *inner = pool_get(parser);
    parse_block(inner, blob_str(temp), blob_len(temp), parser);
    pool_put(parser, temp);
    temp = inner;
  }
  // else too deep: do not parse inner, just render as text

  if (parser->render.blockquote)
    parser->render.blockquote(out, temp, parser->udata);
  pool_put(parser, temp);
  return i;
}


/** check for fenced code block; return #ticks or 0 */
static size_t
is_fenceline(const char *text, size_t size)
{
  size_t pre, i;
  char delim;
  pre = preblanks(text, size);
  if (pre >= size) return 0;
  i = pre;
  delim = text[i];
  if (delim != '`' && delim != '~') return 0;
  for (++i; i < size && text[i] == delim; i++);
  if (i - pre < 3) return 0; /* need at least 3 ticks */
  /* if started by ticks: no more ticks on this line: */
  if (delim == '`') {
    while (i < size && text[i] != '\n' && text[i] != '\r') {
      if (text[i] == delim) return 0;
      i++;
    }
  }
  return i - pre;
}

// inline code: n ticks, code, n ticks; trim code; n >= 1
// block code: n ticks, code n+ ticks, nl; info string; trim code; n >= 3

static size_t
parse_fencedcode(Blob *out, const char *text, size_t size, Parser *parser)
{
  Blob *temp = pool_get(parser);
  size_t pre, i;
  size_t nopen, nclose;
  size_t infofs, infend;
  char delim;

  pre = preblanks(text, size);
  if (pre >= size) return 0;

  i = pre;
  delim = text[i];
  for (++i; i < size && text[i] == delim; i++);
  nopen = i - pre;

  /* line may has info string, but must not have any more ticks: */
  while (i < size && ISBLANK(text[i])) i++;
  infofs = i;
  for (; i < size && text[i] != '\n' && text[i] != '\r'; i++) {
    if (text[i] == delim && delim == '`') return 0;
  }
  infend = i;
  while (infend > infofs && ISBLANK(text[infend-1])) infend--;
  if (i < size) i++;
  if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;

  while (i < size) {
    size_t start = i, j, end, n;
    while (i < size && text[i] != '\n' && text[i] != '\r') i++;
    end = i;
    if (i < size) i++;
    if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
    /* look for closing delimiters: */
    n = preblanks(text+start, end-start);
    for (nclose = 0, j = start+n; j < end && text[j] == delim; j++, nclose++);
    if (nclose >= nopen) {
      while (j < size && ISBLANK(text[j])) j++;
      if (j >= size || text[j] == '\n' || text[j] == '\r')
        break; /* fenced block ended */
    }
    /* remove indent blanks, if any: */
    if (pre) {
      for (j = 0; j < pre && text[start+j] == ' '; j++);
      start += j;
    }

    blob_addbuf(temp, text+start, end-start);
    blob_addchar(temp, '\n');
  }

  if (parser->render.codeblock) {
    Blob *info = pool_get(parser);
    do_escapes(info, text+infofs, infend-infofs);
    parser->render.codeblock(out, blob_str(info), temp, parser->udata);
    pool_put(parser, info);
  }

  pool_put(parser, temp);
  return i;
}


/** check for hrule line; return length (incl newline) or 0 */
static size_t
is_hrule(const char *text, size_t size)
{
  size_t i, num = 0;
  char c;

  i = preblanks(text, size);
  /* need at least 3 * or - or _ with optional blanks and nothing else */
  if (i+3 >= size) return 0; /* too short for an hrule */
  c = text[i];
  if (c != '*' && c != '-' && c != '_') return 0;
  while (i < size && text[i] != '\n' && text[i] != '\r') {
    if (text[i] == c) num += 1;
    else if (!ISBLANK(text[i])) return 0;
    i += 1;
  }
  if (num < 3) return 0;
  if (i < size) i++;
  if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
  return i;
}


static void
do_hrule(Blob *out, Parser *parser)
{
  if (parser->render.hrule)
    parser->render.hrule(out, parser->udata);
}


/** check for an (ordered or unordered) list item line; return prefix length */
static size_t
is_itemline(const char *text, size_t size, char *ptype, int *pstart)
{
  size_t i, j = preblanks(text, size);
  if (ptype) *ptype = '?';
  if (j+1 >= size) return 0;
  if (text[j] == '*' || text[j] == '+' || text[j] == '-') {
    if (!ISBLANK(text[j+1])) return 0;
    if (ptype) *ptype = text[j];
    return j+2;
  }
  if ('0' <= text[j] && text[j] <= '9') {
    for (i=j++; '0' <= text[j] && text[j] <= '9'; j++);
    if (j+1 >= size) return 0;
    if (text[j] != '.' && text[j] != ')') return 0;
    if (!ISBLANK(text[j+1])) return 0;
    if (ptype) *ptype = text[j];
    if (pstart) *pstart = atoi(text+i);
    return j+2;
  }
  return 0;
}


static size_t
parse_listitem(Blob *out, char type, bool *ploose, const char *text, size_t size, Parser *parser)
{
  Blob *temp = pool_get(parser);
  size_t pre, i, j, len, sublist;
  int start;
  char itemtype;
  bool wasblank;

  assert(ploose != 0);

  /* scan item prefix, remember indent: */
  pre = is_itemline(text, size, &itemtype, &start);
  if (!pre || itemtype != type) return 0; /* list ends */

  j = pre;
  wasblank = false;
  sublist = 0;

  /* scan remainder of first line: */
  for (i=j; j < size && text[j] != '\n' && text[j] != '\r'; j++);
  blob_addbuf(temp, text+i, j-i);
  blob_addchar(temp, '\n');
  if (j < size) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;

  /* scan remaining lines, if any: */
  while (j < size) {
    if ((len = is_blankline(text+j, size-j))) {
      wasblank = true;
      j += len;
      continue;
    }
    /* find indent (up to pre): */
    for (i = 0; i < pre && text[j+i] == ' '; i++);
    /* item ends on next (non-sub) item or less indented material: */
    if (is_itemline(text+j+i, size-j-i, 0, 0) && !is_hrule(text+j+i, size-j-i)) {
      if (wasblank) *ploose = true;
      if (i < pre) break; /* next item ends current item */
      if (!sublist) sublist = blob_len(temp); /* sublist start offset */
    }
    else if (wasblank) {
      if (i < pre) {
        // TODO flag list done to caller!?
        break;
      }
      else {
        blob_addchar(temp, '\n');
        *ploose = true;
      }
    }

    for (j+=i, i=j; j < size && text[j] != '\n' && text[j] != '\r'; j++);
    blob_addbuf(temp, text+i, j-i);
    blob_addchar(temp, '\n');
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    wasblank = false;
  }

  /* item is now in temp blob, including subitems, if any, */
  /* render it into the inner buffer: */
  Blob *inner = pool_get(parser);
  if (!*ploose || too_deep(parser)) {
    if (sublist && sublist < blob_len(temp)) {
      parse_inlines(inner, blob_buf(temp), sublist, parser);
      parse_block(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, parser);
    }
    else {
      parse_inlines(inner, blob_buf(temp), blob_len(temp), parser);
    }
  }
  else {
    if (sublist && sublist < blob_len(temp)) {
      /* Want two blocks: the stuff before, and then the sublist */
      parse_block(inner, blob_buf(temp), sublist, parser);
      parse_block(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, parser);
    }
    else {
      parse_block(inner, blob_buf(temp), blob_len(temp), parser);
    }
  }

  if (parser->render.listitem)
    parser->render.listitem(out, inner, parser->udata);

  pool_put(parser, inner);
  pool_put(parser, temp);

  return j;
}


static size_t
parse_list(Blob *out, char type, int start, const char *text, size_t size, Parser *parser)
{
  Blob *temp = pool_get(parser);
  size_t i, len;
  bool loose = false;

  /* list is sequence of list items ofblob the same type: */
  for (i = 0; i < size; ) {
    len = parse_listitem(temp, type, &loose, text+i, size-i, parser);
    if (!len) break;
    i += len;
  }

  if (parser->render.list)
    parser->render.list(out, type, start, temp, parser->udata);
  pool_put(parser, temp);
  return i;
}


static size_t
is_htmlline(const char *text, size_t size, int *pkind, Parser *parser)
{
  // 1. <pre, <script, <style, or <textarea ... until closing tag
  // 2. <!-- ... until -->
  // 3. <? ... until ?>
  // 4. <!LETTER ... until >
  // 5. <![CDATA[ ... until ]]>
  // 6. < or </ followed by one of (case-insensitive) address, article,
  //    aside, base, basefont, blockquote, body, caption, center, col,
  //    colgroup, dd, details, dialog, dir, div, dl, dt, fieldset,
  //    figcaption, figure, footer, form, frame, frameset,
  //    h1, h2, h3, h4, h5, h6, head, header, hr, html, iframe, legend,
  //    li, link, main, menu, menuitem, nav, noframes, ol, optgroup,
  //    option, p, param, section, source, summary, table, tbody, td,
  //    tfoot, th, thead, title, tr, track, ul;
  //    followed by a space, a tab, the end of the line, the string >,
  //    or the string /> ... until a blank line
  // 7. a complete open tag (but not one from case 1), until blank line

  const struct tagname *name;
  size_t len, i, j = preblanks(text, size);
  bool isclose;

  if (pkind) *pkind = 0;
  if (j+1 >= size) return 0;
  if (text[j] != '<') return 0;
  if (text[j+1] == '?') {
    if (pkind) *pkind = 3; /* processing instruction */
    return j+2;
  }
  if (text[j+1] == '!' && j+2 < size) {
    /* look for "<!--" (comment), ended by "-->" */
    if (j+3 < size && text[j+2] == '-' && text[j+3] == '-') {
      if (pkind) *pkind = 2;
      return j+4;
    }
    /* look for "<!X" (declaration), ended by ">" */
    if (j+2 < size && ISALPHA(text[j+2])) {
      if (pkind) *pkind = 4;
      return j+3;
    }
    /* look for "<![CDATA[", ended by "]]>" */
    if (j+8 < size && strncmp("[CDATA[", text+j+2, 7) == 0) {
      if (pkind) *pkind = 5;
      return j+9;
    }
    return 0;
  }
  i = j++;  /* skip the '<' */
  isclose = text[j] == '/';
  if (isclose) j++;
  name = tagname_find(text+j, size-j);
  if (name) {
    bool istext = name == parser->pretag || name == parser->scripttag ||
                  name == parser->styletag || name == parser->textareatag;
    j += name->len;
    if (!isclose && istext && j < size && (text[j] == '>' || ISSPACE(text[j]))) {
      if (pkind) *pkind = 1;
      return j+1;
    }
    else if (j < size && (text[j] == '>' || ISSPACE(text[j]) ||
                         (j+1 < size && text[j] == '/' && text[j] == '>'))) {
      if (pkind) *pkind = 6;
      return text[j] == '/' ? j+1 : j+2;
    }
    return 0;
  }
  if ((len = taglength(text+i, size-i))) {
    if (pkind) *pkind = 7;
    return j + len;
  }
  return 0;
}


static size_t
parse_htmlblock(Blob *out, const char *text, size_t size, size_t startlen, int kind, Parser *parser)
{
  // Assume text points to "start condition" of the given kind (CM 4.6)
  // and startlen is the length of this start condition. End condition is:
  // kind=1: </pre> or </script> or </style> or </textarea>
  // kind=2:    -->
  // kind=3:     ?>       For kinds 1..5 scan to '>' and look back
  // kind=4:      >       for the specifics
  // kind=5:    ]]>
  // kind=6,7: blank line

  size_t j = startlen;

  if (kind == 6 || kind == 7) {
    while (j < size) {
      size_t len = scanline(text+j, size-j);
      j += len;
      if (is_blankline(text+j, size-j)) break;
    }
    blob_addbuf(out, text, j);
    return j;
  }

  switch (kind) {
    case 1: j += 6; break;
    case 2: j += 3; break;
    case 3: j += 2; break;
    case 4: j += 1; break;
    case 5: j += 3; break;
    default: assert(INVALID_HTMLBLOCK_KIND); break;
  }

  while (j < size) {
    const char *p = memchr(text+j, '>', size-j);
    if (!p) { j = size; break; } /* not found: to end of document or block */
    j = p - text + 1;
    if ((kind == 2 && p >= text+j+2 && p[-2] == '-' && p[-1] == '-') ||
        (kind == 3 && p >= text+j+1 &&                 p[-1] == '?') ||
        (kind == 4)                                                  ||
        (kind == 5 && p >= text+j+2 && p[-2] == ']' && p[-1] == ']') ||
        (kind == 1 && (!memcmp("</pre", p-5, 5) || !memcmp("</script", p-8, 8) ||
                       !memcmp("</style", p-7, 7) || !memcmp("</textarea", p-10, 10)))) {
      break;  /* end condition found */
    }
  }

  if (parser->render.htmlblock) {
    Blob html = { (char *) text, j, 0 }; /* static */
    parser->render.htmlblock(out, &html, parser->udata);
  }

  return j;
}


static size_t
parse_paragraph(Blob *out, const char *text, size_t size, Parser *parser)
{
  Blob *temp = pool_get(parser);
  const char *s;
  size_t i, j, len, n;
  int level;

  for (i = n = 0; i < size; ) {
    len = scanline(text+i, size-i);
    if (!len) break;
    if (is_blankline(text+i, len)) break;
    if (is_atxline(text+i, len, 0)) break;
    if ((n=is_setextline(text+i, len, &level))) break;
    if (is_hrule(text+i, len)) break;
    if (is_fenceline(text+i, len)) break;
    if (is_itemline(text+i, len, 0, 0)) break;
    for (j = 0; j < len && ISBLANK(text[i+j]); j++);
    blob_addbuf(temp, text+i+j, len-j);
    /* NOTE trailing blanks and breaks handled by inline parsing */
    i += len;
  }

  s = blob_str(temp);
  j = blob_len(temp);
  while (j > 0 && (s[j-1] == '\n' || s[j-1] == '\r')) j--;
  blob_trunc(temp, j);

  if (level > 0 && blob_len(temp) > 0) {
    if (parser->render.heading) {
      Blob *title = pool_get(parser);
      parse_inlines(title, blob_buf(temp), blob_len(temp), parser);
      parser->render.heading(out, level, title, parser->udata);
      pool_put(parser, title);
    }
    i += n; /* consume the setext underlining */
  }
  else if (i > 0 && parser->render.paragraph) {
    Blob *para = pool_get(parser);
    parse_inlines(para, blob_buf(temp), blob_len(temp), parser);
    parser->render.paragraph(out, para, parser->udata);
    pool_put(parser, para);
  }

  pool_put(parser, temp);

  return i;
}


static void
parse_block(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t start;
  char itemtype;
  int itemstart;
  int htmlkind;

  parser->nesting_depth++;
  for (start=0; start<size; ) {
    const char *ptr = text+start;
    size_t len, end = size-start;

    if ((len = parse_atxheading(out, ptr, end, parser))) {
      /* nothing else to do */
    }
    else if (is_codeline(ptr, end)) {
      len = parse_codeblock(out, ptr, end, parser);
    }
    else if (is_quoteline(ptr, end)) {
      len = parse_blockquote(out, ptr, end, parser);
    }
    else if ((len = is_hrule(ptr, end))) {
      do_hrule(out, parser);
    }
    else if (is_fenceline(ptr, end)) {
      len = parse_fencedcode(out, ptr, end, parser);
    }
    else if (is_itemline(ptr, end, &itemtype, &itemstart)) {
      len = parse_list(out, itemtype, itemstart, ptr, end, parser);
    }
    else if ((len = is_htmlline(ptr, end, &htmlkind, parser))) {
      len = parse_htmlblock(out, ptr, end, len, htmlkind, parser);
    }
    // TODO table lines
    else if ((len = is_linkdef(ptr, end, 0))) {
      /* nothing to do, just skip it */
    }
    else if ((len = is_blankline(ptr, end))) {
      /* nothing to do, blank lines separate blocks */
    }
    else {
      /* Non-blank lines that cannot be interpreted otherwise
         form a paragraph in Markdown/CommonMark: */
      len = parse_paragraph(out, ptr, end, parser);
    }

    if (len) start += len;
    else break;
  }
  parser->nesting_depth--;
}


static void
init(Parser *parser, struct markdown *mkdn)
{
  assert(parser != 0);
  assert(mkdn != 0);
  parser->render = *mkdn;
  parser->udata = mkdn->udata;
  parser->nesting_depth = 0;
  parser->in_link = 0;
  parser->linkdefs = (Blob) BLOB_INIT;
  parser->pool_index = 0;
  memset(parser->blob_pool, 0, sizeof(parser->blob_pool));
  memset(parser->active_chars, 0, sizeof(parser->active_chars));
  parser->active_chars['\\'] = do_escape;
  parser->active_chars['<'] = do_anglebrack;
  if (mkdn->entity) parser->active_chars['&'] = do_entity;
  if (mkdn->linebreak) parser->active_chars['\n'] = do_linebreak;
  if (mkdn->linebreak) parser->active_chars['\r'] = do_linebreak;
  if (mkdn->codespan) parser->active_chars['`'] = do_codespan;
  if (mkdn->image || mkdn->link) parser->active_chars['['] = do_link;

  parser->pretag = tagname_find("pre", -1);
  assert(parser->pretag != 0);
  parser->scripttag = tagname_find("script", -1);
  assert(parser->scripttag != 0);
  parser->styletag = tagname_find("style", -1);
  assert(parser->styletag != 0);
  parser->textareatag = tagname_find("textarea", -1);
  assert(parser->textareatag != 0);
}


void
markdown(Blob *out, const char *text, size_t size, struct markdown *mkdn)
{
  size_t start;
  Parser parser;

  if (!text || !size || !mkdn) return;
  assert(out != NULL);

  init(&parser, mkdn);

  /* 1st pass: collect references */
  for (start = 0; start < size; ) {
    Linkdef linkdef;
    size_t len = is_linkdef(text+start, size-start, &linkdef);
    if (len) {
      start += len;
      blob_addbuf(&parser.linkdefs, (void*)&linkdef, sizeof(linkdef));
    }
    else {
      // TODO pre process into buffer: expand tabs (size=4), newlines to \n, \0 to U+FFFD
      len = scanline(text+start, size-start);
      if (len) start += len;
      else break;
    }
  }

  if (blob_len(&parser.linkdefs) > 0) {
    qsort(blob_buf(&parser.linkdefs),
          blob_len(&parser.linkdefs)/sizeof(struct linkdef),
          sizeof(struct linkdef), linkdef_cmp);
  }

  /* 2nd pass: do the rendering */
  if (mkdn->prolog) mkdn->prolog(out, mkdn->udata);
  parse_block(out, text, size, &parser);
  if (mkdn->epilog) mkdn->epilog(out, mkdn->udata);

  /* cleanup */
  assert(parser.nesting_depth == 0);
  blob_free(&parser.linkdefs);
  while (parser.pool_index > 0) {
    blob_free(parser.blob_pool[--parser.pool_index]);
  }
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#ifdef MKDN_SHELL

int
main(int argc, char **argv)
{
  Blob input = BLOB_INIT;
  Blob output = BLOB_INIT;
  char buf[2048];
  size_t n;

  if (argc > 1) {
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
      blob_addbuf(&input, buf, n);
    }
    fclose(fp);
  }
  else {
    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
      blob_addbuf(&input, buf, n);
    }
  }

  mkdnhtml(&output, blob_str(&input), blob_len(&input), 0, 256);

  fputs(blob_str(&output), stdout);
  fflush(stdout);

  blob_free(&input);
  blob_free(&output);

  return 0;
}

#endif
