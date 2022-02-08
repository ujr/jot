
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "log.h"
#include "memory.h"
#include "mkdn.h"
#include "utils.h"


/* Markdown has block elements and inline (span) elements.
 * Reference style links (and images) are forward-looking,
 * so we need to collect link definitions before the actual
 * rendering in a second pass over the Markdown. The second
 * pass will parse the input into a sequence of "blocks"
 * and, within each block, parse "inlines" such as emphasis
 * or links. Some block types can contain other blocks.
 *
 * The parser here is inspired by the one in Fossil SCM,
 * which itself derives from the parser by Natacha Porté,
 * but inline parsing follows the algorithm sketched in
 * the CommonMark specification.
 */


#define ISASCII(c) (0 <= (c) && (c) <= 127)
#define ISCNTRL(c) ((0 <= (c) && (c) < 32) || (c) == 127)
#define ISPUNCT(c) (('!' <= (c) && (c) <= '/')  /* 21..2F  !"#$%&'()*+,-./ */ \
                 || (':' <= (c) && (c) <= '@')  /* 3A..40  ; ; < = > ? @   */ \
                 || ('[' <= (c) && (c) <= '`')  /* 5B..60  [ \ ] ^ _ `     */ \
                 || ('{' <= (c) && (c) <= '~')) /* 7B..7E  { | } ~         */
#define ISSPACE(c) ((c) == ' ' || ('\t' <= (c) && (c) <= '\r'))
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#define ISDIGIT(c) ('0' <= (c) && (c) <= '9')
#define ISXDIGIT(c) (ISDIGIT(c) || ('a' <= (c) && (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))
#define ISLOWER(c) ('a' <= (c) && (c) <= 'z')
#define ISUPPER(c) ('A' <= (c) && (c) <= 'Z')
#define ISALPHA(c) (ISLOWER(c) || ISUPPER(c))
#define ISALNUM(c) (ISALPHA(c) || ISDIGIT(c))
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

#define PUBLIC  /* tag a function */
#define UNUSED(x) ((void)(x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* for meaningful assertion messages */
#define OUT_OF_MEMORY 0
#define INVALID_HTMLBLOCK_KIND 0
#define NOT_REACHED 0

/* length of array (number of items) */
#define ARLEN(a) (sizeof(a)/sizeof((a)[0]))


typedef struct parser Parser;

typedef size_t (*CharProc)(
  Blob *out, const char *text, size_t pos, size_t size, Parser *parser);

struct parser {
  struct markdown render;
  void *udata;
  int debug;
  int nesting_depth;
  int in_link;               /* to inhibit links within links */
  Blob linkdefs;             /* collected link definitions */
  Blob *blob_pool[32];
  size_t pool_index;
  CharProc livechars[128];   /* chars like & and \ that trigger an action */
  char emphchars[32];        /* all chars that mark emphasis */
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


/** html block tags, ordered as by tagname_cmp (first len, then lexic) */
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


/* === housekeeping === */

static Blob*
blob_get(Parser *parser)
{
  static const Blob empty = BLOB_INIT;
  Blob *ptr;
  if (parser->pool_index > 0)
    return parser->blob_pool[--parser->pool_index];
  ptr = mem_alloc(sizeof(*ptr));
  assert(ptr != OUT_OF_MEMORY);
  *ptr = empty;
  return ptr;
}


static void
blob_put(Parser *parser, Blob *blob)
{
  if (!blob) return;
  if (parser->pool_index < ARLEN(parser->blob_pool)) {
    blob_clear(blob);
    parser->blob_pool[parser->pool_index++] = blob;
  }
  else {
    log_debug("mkdn: blob pool exhausted; freeing blob instead of caching");
    blob_free(blob);
    mem_free(blob);
  }
}


static bool
too_deep(Parser *parser)
{
  return parser->nesting_depth > 100;
}


/* === lexical scanning === */


/** scan a run of white space */
static size_t
scan_space(const char *text, size_t size)
{
  size_t j = 0;
  while (j < size && ISSPACE(text[j])) j++;
  return j;
}


/** scan white space but at most one newline */
static size_t
scan_innerspace(const char *text, size_t size)
{
  size_t j = 0;
  while (j < size && ISBLANK(text[j])) j++;
  if (j < size && (text[j] == '\n' || text[j] == '\r')) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  while (j < size && ISBLANK(text[j])) j++;
  return j;
}


static size_t
scan_comment(const char *text, size_t size)
{
  size_t j;
  if (size < 7) return 0;  /* <!----> is shortest comment */
  if (text[0] != '<' || text[1] != '!' ||
      text[2] != '-' || text[3] != '-') return 0;
  for (j = 6; j < size; ) {
    const char *p = memchr(text+j, '>', size-j);
    if (!p) break;
    j = p - text + 1;
    if (p[-1] == '-' && p[-2] == '-') return j;
  }
  return 0;
}


static size_t
scan_procinst(const char *text, size_t size)
{
  size_t j;
  if (size < 4) return 0;  /* <??> is shortest proc inst */
  if (text[0] != '<' || text[1] != '?') return 0;
  for (j = 3; j < size; ) {
    const char *p = memchr(text+j, '>', size-j);
    if (!p) break;
    j = p - text + 1;
    if (p[-1] == '?') return j;
  }
  return 0;
}


static size_t
scan_cdata(const char *text, size_t size)
{
  size_t j;
  if (size < 12) return 0;  /* <![CDATA[]]> is shortest */
  if (strncmp("<![CDATA[", text, 8) != 0) return 0;
  for (j = 12; j < size; ) {
    const char *p = memchr(text+j, '>', size-j);
    if (!p) break;
    j = p - text + 1;
    if (p[-2] == ']' && p[-1] == ']') return j;
  }
  return 0;
}


/** return length of HTML tag at text or 0 if malformed */
static size_t
scan_tag(const char *text, size_t size, bool oneline)
{
  size_t len;
  int closing = 0, quote, c;
  /* <x> is shortest possible tag */
  if (size < 3) return 0;
  if (text[0] != '<') return 0;

  if (text[1] == '?')
    return scan_procinst(text, size);
  if (text[1] == '!' && text[2] == '-')
    return scan_comment(text, size);
  if (text[1] == '!' && text[2] == '[')
    return scan_cdata(text, size);

  len = 1;
  if (text[1] == '/') { ++len; closing=1; }
  else if (text[1] == '!') { ++len; }  /* treat decls as regular tags */
  if (!ISALPHA(text[len])) return 0;
  while (len < size && (ISALNUM(text[len]) || text[len] == '-')) len++;
  if (oneline) while (len < size && ISBLANK(text[len])) len++;
  else len += scan_innerspace(text+len, size-len);
  if (len >= size) return 0;
  if (text[len] == '>') return len+1;
  if (closing) return 0; /* closing tag cannot have attrs */
  if (text[len] == '/') {
    len += 1;
    return len < size && text[len] == '>' ? len+1 : 0;
  }
  if (!ISSPACE(text[len-1])) return 0;  /* attrs must be separated */
  /* scan over (possibly quoted) attributes: */
  quote = 0;
  while (len < size && (c=text[len]) != 0 && (c != '>' || quote)) {
    if (c == quote) quote = 0;
    else if (!quote && (c == '"' || c == '\'')) quote = c;
    else if ((c == '\n' || c == '\r') && oneline) return 0;
    len++;
  }
  if (len >= size || text[len] != '>') return 0;
  return len+1;
}


/** scan past next newline; return #bytes scanned */
static size_t
scan_line(const char *text, size_t size)
{
  size_t j = 0;
  while (j < size && text[j] != '\n' && text[j] != '\r') j++;
  if (j < size) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  return j;
}


/** return length (or 0) and type (':' link, '@' mail, '?' mismatch) */
static size_t
scan_autolink(const char *text, size_t size, char *ptype)
{
  /* by CM 6.5 an autolink is either an absolute URI or a mail addr in
     angle brackets; absolute URI has scheme then ':' then anything but
     space nor < nor > nor cntrl; scheme is [A-Za-z][A-Za-z0-9+.-]{1,31};
     mail addr is defined by a monstrous regex, which we oversimplify here;
     backslashes are NOT expanded in autolinks; note that raw HTML tags
     are distinguished by NOT having a : nor @ in tag name */

  static const char *extra = "@.-+_=.~";  /* allowed in mail beyond alnum */
  size_t j, max, nat;

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
    goto trymail;
  }
  for (; j < size && text[j] != '>'; j++) {
    if (ISSPACE(text[j]) || text[j] == '<' || ISCNTRL(text[j])) return 0;
  }
  if (j >= size || text[j] != '>') return 0;
  if (ptype) *ptype = ':';  /* hyperlink */
  return j+1;

trymail:
  /* scan over "reasonable" characters until '>'; mail if exactly one '@': */
  for (nat = 0; j < size && (ISALNUM(text[j]) || strchr(extra, text[j])); j++) {
    if (text[j] == '@') nat++;
  }
  if (j >= size || text[j] != '>' || nat != 1) return 0;
  if (ptype) *ptype = '@';
  return j+1;
}


/** scan link label like [foo]; return length or zero */
static size_t
scan_link_label(const char *text, size_t size)
{
  size_t j = 0;
  if (j >= size || text[j] != '[') return 0;
  for (++j; j < size; j++) {
    if (text[j] == ']') break;  /* terminates if unescaped */
    if (text[j] == '[') return 0;  /* illegal if unescaped */
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

  /* ensure out params are defined: */
  if (link) *link = slice(text, 0);
  if (title) *title = slice(text, 0);

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

  /* need a space or end of input (linkdef) or closing paren (inline): */
  if (j < size && !ISSPACE(text[j]) && text[j] != ')') return 0;
  j += scan_innerspace(text+j, size-j);

  /* optional title, may contain newlines, but no blank line: */
  titlofs = titlend = 0;
  if (j < size && (text[j] == '"' || text[j] == '\'' || text[j] == '(')) {
    char delim = text[j++], blank = 0;
    if (delim == '(') delim = ')';
    titlofs = j;
    for (; j < size && text[j] != delim; j++) {
      if (text[j] == '\\') { j++; continue; }
      if (text[j] == '\n' || text[j] == '\r') {
        if (blank) return 0;  /* empty line in title not allowed */
        blank = 1;
      }
      else if (!ISBLANK(text[j])) blank = 0;
    }
    titlend = j;
    if (titlend < titlofs || text[j] != delim) return 0;
    end = ++j;  /* consume the closing delim */
  }

  if (link) *link = slice(text+linkofs, linkend-linkofs);
  if (title) *title = slice(text+titlofs, titlend-titlofs);

  return end;
}


static size_t
scan_inline_link(const char *text, size_t size,
  struct slice *linkslice, struct slice *titleslice)
{
  size_t j = 0, len;
  if (size < 2 || text[j] != '(') return 0;
  for (++j; j < size && ISBLANK(text[j]); j++);
  len = scan_link_and_title(text+j, size-j, linkslice, titleslice);
  if (!len && text[j] != ')') return 0;  /* allow empty () */
  j += len;
  while (j < size && ISBLANK(text[j])) j++;
  return j < size && text[j] == ')' ? j+1 : 0;  /* expect and eat closing paren */
}


/** scan the link part after [body], which may be empty */
static size_t
scan_link_tail(
  const char *text, size_t size, size_t bodyofs, size_t bodyend,
  Parser *parser, struct slice *linkslice, struct slice *titleslice)
{
  /* There are our types of links: all start with [body], which was
  // already parsed and is available through bodyofs and bodyend:
  // - inline:    [text](dest "title")
  // - reference: [text][ref]
  // - collapsed: [text][]     let ref = text
  // - shortcut:  [text]       let ref = text
  //       bodyofs^     ^bodyend
  */
  size_t j = bodyend+1, len;
  const char *bodyptr = text+bodyofs+1;
  size_t bodylen = bodyend-bodyofs-1;

  /* bodyofs points to opening bracket, bodyend to closing bracket */
  assert(bodyofs < bodyend && bodyend <= size);
  /* link body [foo] was already parsed, look for the rest:
     inline (...), reference [label], collapsed [], shortcut */

  if (j < size && text[j] == '(') {  /* inline link */
    len = scan_inline_link(text+j, size-j, linkslice, titleslice);
    if (len) return j+len;
    /* else: could still be a reference or shortcut link! */
  }

  if (j < size && text[j] == '[') {
    len = scan_link_label(text+j, MIN(size-j, 999));
    if (len < 2) return 0;
    if (!parser) return j+len;
    if (len == 2) {  /* collapsed link: [] */
      if (!linkdef_find(bodyptr, bodylen, parser, linkslice, titleslice))
        return 0;
    }
    else {  /* reference link: [ref] */
      if (!linkdef_find(text+j+1, len-2, parser, linkslice, titleslice))
        return 0;
    }
    return j + len;
  }
  /* shortcut link (no label at all, just text in brackets) */
  if (!parser) return j;
  return linkdef_find(bodyptr, bodylen, parser, linkslice, titleslice) ? j : 0;
}


#if 0
static size_t
scan_link_body(const char *text, size_t size)
{
  size_t j = 0;
  int level;

  if (j >= size || text[j] != '[') return 0;
  /* allow balanced brackets while looking for closing bracket: */
  for (level = 1, ++j; j < size; j++) {
    if (text[j] == '\\') { j++; continue; }
    if (text[j] == '[') level += 1;
    else if (text[j] == ']') {
      if (--level <= 0) break;
    }
  }
  if (j >= size) return 0;  /* unmatched '[' */
  return j+1;  /* include closing bracket */
}


/** scan full link: inline, reference, collapsed, or shortcut */
static size_t
scan_full_link(const char *text, size_t size, Parser *parser,
  struct slice *bodyslice, struct slice *linkslice, struct slice *titleslice)
{
  const char *bodyptr;
  size_t j = 0, len, bodylen;

  if (size < 2 || text[j] != '[') return 0;
  len = scan_link_body(text, size);
  if (!len) return 0;
  j += len;

  bodyptr = text+1;
  bodylen = len-2;
  if (bodyslice) *bodyslice = slice(bodyptr, bodylen);

  return scan_link_tail(text, size, 0, len, parser, linkslice, titleslice);
}
#endif


static size_t
scan_tickrun(const char *text, size_t size)
{
  size_t j;
  if (!size) return 0;
  for (j=1; j < size && text[j] == text[0]; j++);
  return j;
}


static size_t
scan_codespan(const char *text, size_t size, struct slice *codeslice)
{
  /* text enclosed in backtick strings of equal length;
     if blanks at start AND end, trim ONE on each side */
  size_t j, oticks, cticks;
  char delim;

  if (size < 2) return 0;
  delim = text[0];
  for (j = 1; j < size && text[j] == delim; j++);
  oticks = j;

  for (cticks = 0; j < size; j++) {
    if (text[j] == delim) cticks += 1;
    else if (cticks == oticks) break;
    else cticks = 0;
    // TODO if blank line: stop (not a codespan)
  }

  if (cticks != oticks) return 0;

  if (codeslice) {
    size_t i = oticks, k = j-cticks;
    /* trim one blank if begins&ends with blank (or newline): */
    if (i+2 < k && ISSPACE(text[i]) && ISSPACE(text[k-1])) {
      i += 1; k -= 1;
    }
    *codeslice = slice(text+i, k-i);
  }

  return j;
}


PUBLIC size_t
scan_entity(const char *text, size_t size)
{
  size_t i, j = 0;
  if (j >= size || text[j] != '&') return 0;
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
  if (j >= size || text[j] != ';') return 0;
  return j+1;
}


// static void
// do_escapes(Blob *out, const char *text, size_t size)
// {
//   size_t i, j;
//   for (i = j = 0; j < size; ) {
//     for (; j < size; j++) {
//       if (text[j] == '\\') break;
//     }
//     if (j > i) {
//       blob_addbuf(out, text+i, j-i);
//       i = j;
//     }
//     if (j >= size) break;
//     assert(i == j);
//     assert(text[j] == '\\');
//     j += 1;  /* leave i unchanged to add \ to run of text */
//     if (j < size && ISPUNCT(text[j])) {
//       blob_addchar(out, text[j]);
//       i = j += 1;
//     }
//   }
// }


/** resolve escapes and entities */
static void
emit_url(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t i, j;
  for (j = 0; ; ) {
    for (i = j; j < size; j++) {
      if (text[j] == '\\') break;
      if (text[j] == '&') break;
    }

    if (j > i) blob_addbuf(out, text+i, j-i);
    if (j >= size) break;

    if (text[j] == '\\') {
      if (j+1 < size && ISPUNCT(text[j+1])) {
        blob_addchar(out, text[j+1]);
        j += 2;
        continue;
      }
      blob_addchar(out, '\\');
      j += 1;
      continue;
    }

    if (text[j] == '&') {
      size_t len = scan_entity(text+j, size-j);
      if (len && parser->render.entity &&
          parser->render.entity(out, text+j, len, parser->udata)) {
        j += len;
        continue;
      }
      blob_addchar(out, '&');
      j += 1;
      continue;
    }

    assert(NOT_REACHED);
  }
}


static size_t
emit_escape(
  Blob *out, const char *text, size_t pos, size_t size, Parser *parser)
{
  assert(pos < size);
  pos += 1;
  /* by CM 2.4, backslash escapes only punctuation, otherwise literal */
  if (pos < size && ISPUNCT(text[pos])) {
    if (parser->render.text)
      parser->render.text(out, text+pos, 1, parser->udata);
    else blob_addbuf(out, text+pos, 1);
    return 2;
  }
  return 0;
}


static size_t
emit_entity(
  Blob *out, const char *text, size_t pos, size_t size, Parser *parser)
{
  /* by CM 2.5, only valid HTML5 entity names must be parsed; here we
     only parse for syntactical correctness and let the callback decide: */
  size_t len = scan_entity(text+pos, size-pos);
  if (!len) return 0;
  assert(parser->render.entity != 0);
  return parser->render.entity(out, text+pos, len, parser->udata) ? len : 0;
}


static size_t
emit_linebreak(
  Blob *out, const char *text, size_t pos, size_t size, Parser *parser)
{
  size_t extra = (text[pos] == '\r' && pos+1 < size && text[pos+1] == '\n') ? 1 : 0;

  /* look back, if possible, for two blanks */
  if (pos >= 2 && text[pos-1] == ' ' && text[pos-2] == ' ') {
    blob_trimend(out); blob_addchar(out, ' ');
    assert(parser->render.linebreak != 0);
    return parser->render.linebreak(out, parser->udata) ? 1 + extra : 0;
  }

  /* NB. backslash newline is NOT handled by do_escape */
  if (pos >= 1 && text[pos-1] == '\\') {
    blob_trunc(out, blob_len(out)-1); blob_addchar(out, ' ');
    assert(parser->render.linebreak != 0);
    return parser->render.linebreak(out, parser->udata) ? 1 + extra : 0;
  }

  /* trim trailing space and preserve "soft" breaks (CM 6.8): */
  blob_trimend(out);
  blob_addchar(out, '\n');
  return 1 + extra;
}


static void
emit_text(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t i, j, n;
  CharProc action;
  /* copy chars to output, looking for "active" chars */
  i = j = 0;
  for (;;) {
    for (; j < size; j++) {
      unsigned uc = ((unsigned) text[j]) & 0x7F;
      action = parser->livechars[uc];
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
    n = action(out, text, j, size, parser);
    if (n) { j += n; i = j; } else j += 1;
  }
}


static void
emit_codespan(Blob *out, const char *text, size_t size)
{
  size_t i, j;
  for (j=0;;) {
    for (i=j; j < size && text[j] != '\n' && text[j] != '\r'; j++);
    if (j > i) {
      blob_addbuf(out, text+i, j-i);
      i = j;
    }
    if (j >= size) break;
    blob_addchar(out, ' ');
    if (++j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  }
}


/* === tree of inline spans === */


typedef struct span Span;
typedef struct spantree SpanTree;

struct span {
  char type;            /* emph or link or ... */
  size_t olen, clen;    /* delimiter length (open and close) */
  size_t ofs, len;      /* stretch of text span */
  size_t down, next;    /* tree linkage */
  struct slice href, title;  /* payload for links */
};

struct spantree {
  Blob *blob;           /* node storage */
};

#if 0
static void
dump_spans(SpanTree *tree)
{
  size_t i;
  size_t n = blob_len(tree->blob)/sizeof(struct span) - 1;
  struct span *spans = blob_buf(tree->blob);
  for (i = 1; i <= n; i++) {
    fprintf(stderr, "%2zu: '%c' ofs=%zu len=%zu, %zu+%zu, down=%zu, next=%zu\n",
      i, spans[i].type, spans[i].ofs, spans[i].len,
      spans[i].olen, spans[i].clen, spans[i].down, spans[i].next);
  }
}
#endif

/** make span tree; add root span; all spans must be inside root */
static void
initspans(SpanTree *tree, size_t ofs, size_t len, Parser *parser)
{
  struct span *span;
  tree->blob = blob_get(parser);
  assert(blob_len(tree->blob) == 0);

  /* span at index 0 is never used: */
  span = blob_prepare(tree->blob, sizeof(*span));
  memset(span, 0, sizeof(*span));
  blob_addlen(tree->blob, sizeof(*span));

  /* span at index 1 is the root: */
  span = blob_prepare(tree->blob, sizeof(*span));
  memset(span, 0, sizeof(*span));
  span->type = 'R';
  span->ofs = ofs;
  span->len = len;
  blob_addlen(tree->blob, sizeof(*span));
}

static void
freespans(SpanTree *tree, Parser *parser)
{
  blob_put(parser, tree->blob);
}

static bool
span_contains(struct span *spans, size_t i, size_t j)
{
  size_t test = spans[j].ofs;
  return i && spans[i].ofs <= test && test < spans[i].ofs+spans[i].len;
}

static bool
span_before(struct span *spans, size_t i, size_t j)
{
  return i && spans[i].ofs + spans[i].len <= spans[j].ofs;
}

static struct span *
addspan(SpanTree *tree, char type, size_t ofs, size_t len, size_t olen, size_t clen)
{
  size_t count, cur, new, parent, left;
  struct span *span, *spans;
  static const size_t root = 1;

  span = blob_prepare(tree->blob, sizeof(*span));
  memset(span, 0, sizeof(*span));
  span->type = type;
  span->ofs = ofs;
  span->len = len;
  span->olen = olen;
  span->clen = clen;
  span->down = span->next = 0;
  blob_addlen(tree->blob, sizeof(*span));

  spans = blob_buf(tree->blob);
  count = blob_len(tree->blob)/sizeof(*span);

  cur = root;
  new = count-1;  /* the new span */
  parent = left = 0;
  while (cur) {
    if (span_contains(spans, cur, new)) { parent = cur; left = 0; cur = spans[cur].down; }
    else if (span_before(spans, cur, new)) { left = cur; cur = spans[cur].next; }
    else break;
  }
  assert(parent != 0);  /* if not, the root is wrong */
  size_t tail;
  if (left) {
    tail = spans[left].next;
    spans[left].next = new;
  }
  else { /* first child */
    tail = spans[parent].down;
    spans[parent].down = new;
  }
  /* split tail into part inside new & part right of new: */
  if (tail && span_contains(spans, new, tail)) {
    spans[new].down = tail;
    while (spans[tail].next && span_contains(spans, new, spans[tail].next))
      tail = spans[tail].next;
    assert(tail != 0);
    spans[new].next = spans[tail].next;
    spans[tail].next = 0;
  }
  else spans[new].next = tail;

  return span;
}

/* Here we build the span tree as we add spans.
// Since the mem block holding the spans may be
// relocated, pointers are relative (indices).
//
// Alternatively, we could just collect spans and
// make them into a tree afterwords, when there are
// no relocations: use real pointers. Algo sketch:
//
//   qsort array of span nodes by ofs, then len
//   let i = 1  // root
//   for j = 2 .. N do
//     if contains(i, j): last(i) = j; down(i) = j; i = j;
//     else:
//       while not contains(i, j): i = up(i)
//       next(last(i)) = j
//       last(i) = j
//       i = j
*/


/* === inline parsing === */


static void
emit_spans(Blob *out, const char *text, SpanTree *tree, size_t span, Parser *parser);


static void
emit_plain(Blob *out, const char *text, SpanTree *tree, size_t span, Parser *parser)
{
  size_t child;
  struct span *spans = blob_buf(tree->blob);
  size_t ofs = spans[span].ofs + spans[span].olen;
  size_t end = spans[span].ofs + spans[span].len - spans[span].clen;
  char type;

  for (child = spans[span].down; child; child = spans[child].next) {
    if (ofs < spans[child].ofs)
      emit_text(out, text+ofs, spans[child].ofs-ofs, parser);

    type = spans[child].type;
    if (strchr(parser->emphchars, type)) {
      Blob *temp = blob_get(parser);
      emit_plain(temp, text, tree, child, parser);
      emit_text(out, blob_str(temp), blob_len(temp), parser);
      blob_put(parser, temp);
    }
    else if (type == '[' || type == '!') {
      emit_plain(out, text, tree, child, parser);
    }
    else if (type == '`') {
      size_t ofs = spans[child].ofs + spans[child].olen;
      size_t len = spans[child].len - spans[child].olen - spans[child].clen;
      emit_text(out, text+ofs, len, parser);
    }
    else {
      size_t ofs = spans[child].ofs;
      size_t len = spans[child].len;
      emit_text(out, text+ofs, len, parser);
    }

    ofs = spans[child].ofs + spans[child].len;
  }
  if (ofs < end) {
    emit_text(out, text+ofs, end-ofs, parser);
  }
}

// TODO limit recursion depth -- either here or when building tree!

static void
emit_span(Blob *out, const char *text, SpanTree *tree, size_t span, Parser *parser)
{
  bool done = false;
  struct span *spans = blob_buf(tree->blob);
  char type = spans[span].type;
  if (strchr(parser->emphchars, type) && parser->render.emphasis) {
    Blob *temp = blob_get(parser);
    char c = spans[span].type;
    int n = spans[span].olen;
    emit_spans(temp, text, tree, span, parser);  /* nested spans */
    done = parser->render.emphasis(out, c, n, temp, parser->udata);
    blob_put(parser, temp);
  }
  else if ((type == '[' && parser->render.link) ||
           (type == '!' && parser->render.image)) {
    Blob *body = blob_get(parser);
    Blob *link = blob_get(parser);
    Blob *title = blob_get(parser);
    struct slice linkslice = spans[span].href;
    struct slice titleslice = spans[span].title;
    if (type == '!')
      emit_plain(body, text, tree, span, parser);  /* unmark nested spans */
    else
      emit_spans(body, text, tree, span, parser);  /* render nested spans */
    emit_url(link, linkslice.s, linkslice.n, parser);
    emit_text(title, titleslice.s, titleslice.n, parser);
    done = type == '!'
      ? parser->render.image(out, link, title, body, parser->udata)
      : parser->render.link(out, link, title, body, parser->udata);
    blob_put(parser, body);
    blob_put(parser, link);
    blob_put(parser, title);
  }
  else if (type == '`' && parser->render.codespan) {
    Blob *temp = blob_get(parser);
    size_t ofs = spans[span].ofs + spans[span].olen;
    size_t len = spans[span].len - spans[span].olen - spans[span].clen;
    emit_codespan(temp, text+ofs, len);
    done = parser->render.codespan(out, temp, parser->udata);
    blob_put(parser, temp);
  }
  else if ((type == '@' || type == ':') && parser->render.autolink) {
    size_t ofs = spans[span].ofs+1;
    size_t len = spans[span].len-2;
    done = parser->render.autolink(out, type, text+ofs, len, parser->udata);
  }
  else if (type == '<' && parser->render.htmltag) {
    size_t ofs = spans[span].ofs;
    size_t len = spans[span].len;
    done = parser->render.htmltag(out, text+ofs, len, parser->udata);
  }

  if (!done)  /* if not processed, emit as plain text */
    emit_text(out, text+spans[span].ofs, spans[span].len, parser);
}


static void
emit_spans(Blob *out, const char *text, SpanTree *tree, size_t span, Parser *parser)
{
  size_t child;
  struct span *spans = blob_buf(tree->blob);
  size_t ofs = spans[span].ofs + spans[span].olen;
  size_t end = spans[span].ofs + spans[span].len - spans[span].clen;

  for (child = spans[span].down; child; child = spans[child].next) {
    if (ofs < spans[child].ofs)
      emit_text(out, text+ofs, spans[child].ofs-ofs, parser);
    emit_span(out, text, tree, child, parser);
    ofs = spans[child].ofs + spans[child].len;
  }
  if (ofs < end) {
    emit_text(out, text+ofs, end-ofs, parser);
  }
}


/* CM 6.2 defines "left flanking" (can begin) and "right flanking"
// (can end emphasis) delimiter runs based on what's before and after.
// In the table below, '_' represents a space; start of text and end
// of text are to be treated as blanks.
//
//   char     char              is left    is right
//   before   after   example   flanking   flanking
//   ------   -----   -------   --------   --------
//   space    space    _*_        no         no
//   space    punct    _*"        yes        no
//   space    alnum    _*b        yes        no
//   punct    space    "*_        no         yes
//   punct    punct    "*"        yes        yes
//   punct    alnum    "*b        yes        no
//   alnum    space    a*_        no         yes
//   alnum    punct    a*"        no         yes
//   alnum    alnum    a*b        yes        yes
*/

struct delim {   /* delimiter run */
  size_t ofs;    /* start offset and */
  size_t len;    /* length of delimiter run */
  char type;     /* type: '*' or '_' or '[' or ... */
  int flags;     /* whether the run is opener, closer, or both */
  struct delim *prev;
  struct delim *next;
};

struct delimlist {
  struct delim *head;
  struct delim *tail;
  MemPool pool;
};

#define DELIMLIST_INIT { 0, 0, POOL_INIT }

#define DELIM_ACTIVE  1  /* to prevent links within links */
#define DELIM_OPENER  2
#define DELIM_CLOSER  4
#define DELIM_SHIFT   3  /* bits past 1st three are available */

#define DELIM_ISACTIVE(item) ((item)->flags & DELIM_ACTIVE)
#define DELIM_CANOPEN(item)  ((item)->flags & DELIM_OPENER)
#define DELIM_CANCLOSE(item) ((item)->flags & DELIM_CLOSER)

#define DELIM_FLAGS(b,c,a) \
  (delim_canopen(b,c,a) + delim_canclose(b,c,a) + DELIM_ACTIVE)

/* define word characters to be ALNUM and all non-ASCII chars:
   a rough approximation, but works reasonably well with UTF-8: */
#define ISWORD(c) (ISALNUM(c) || !ISASCII(c))

static int
delim_canopen(int before, int delim, int after)
{
  if (ISSPACE(after)) return 0;
  if (ISWORD(after)) return delim == '_' && ISWORD(before) ? 0 : DELIM_OPENER;
  if (ISWORD(before)) return 0;
  return DELIM_OPENER;
}

static int
delim_canclose(int before, int delim, int after)
{
  if (ISSPACE(before)) return 0;
  if (ISWORD(before)) return delim == '_' && ISWORD(after) ? 0 : DELIM_CLOSER;
  if (ISWORD(after)) return 0;
  return DELIM_CLOSER;
}

static struct delim *
delim_push(struct delimlist *list, size_t ofs, size_t len, char type, int flags)
{
  struct delim *node = mem_pool_alloc(&list->pool, sizeof(*node));
  assert(node != OUT_OF_MEMORY);
  memset(node, 0, sizeof(*node));
  node->ofs = ofs;
  node->len = len;
  node->type = type;
  node->flags = flags;

  if (!list->head) list->head = node;
  node->next = 0;
  node->prev = list->tail;
  if (list->tail) list->tail->next = node;
  list->tail = node;
  return node;
}

static void
delim_drop(struct delimlist *list, struct delim *node)
{
  assert(!node->prev || node->prev->next == node);
  assert(node->prev || list->head == node);
  assert(!node->next || node->next->prev == node);
  assert(node->next || list->tail == node);

  if (node->prev) node->prev->next = node->next;
  else list->head = node->next;
  if (node->next) node->next->prev =node->prev;
  else list->tail = node->prev;
}

static bool
is_emph_span(struct delim *opener, struct delim *closer)
{
  if (!opener || !closer) return false;
  if (opener->type != closer->type) return false;
  if (!(opener->flags & DELIM_OPENER)) return false;
  if (!(closer->flags & DELIM_CLOSER)) return false;
  if ((opener->flags & DELIM_CLOSER) || (closer->flags & DELIM_OPENER)) {
    /* Citing from CM 6.2: If one of the delimiters can both open and close
       emphasis, then the sum of the lengths of the delimiter runs containing
       the opening and closing delimiters must not be a multiple of 3 unless
       both lengths are multiples of 3. */
    size_t olen = opener->len, clen = closer->len;
    if ((olen+clen) %3 == 0 && (olen%3 || clen%3)) return false;
  }
  return true;
}

static void
free_delims(struct delimlist *list)
{
  mem_pool_free(&list->pool);
  list->head = list->tail = 0;
}

static void
process_emphasis(struct delimlist *list, struct delim *start, size_t end, SpanTree *tree, Parser *parser)
{
  struct delim *ptr;
  struct delim *closer;

  if (!start) start = list->head;

  for (ptr = start; ptr; ) {
    /* find next potential closer: */
    while (ptr && ptr->ofs < end && !DELIM_CANCLOSE(ptr))
      ptr = ptr->next;
    if (!ptr || ptr->ofs >= end) break; /* no more closers: done */
    closer = ptr;
    /* look back for first matching opener: */
    ptr = ptr->prev;
    while (ptr && ptr != start->prev && !is_emph_span(ptr, closer))
      ptr = ptr->prev;

    if (ptr && ptr != start->prev) { struct delim *opener = ptr;
      size_t m = opener->len >= 2 && closer->len >= 2 ? 2 : 1;
      size_t ofs = opener->ofs + opener->len - m;
      size_t len = closer->ofs + m - ofs;
      addspan(tree, opener->type, ofs, len, m, m);
      /* drop emph delims between opener and closer: */
      for (ptr = opener->next; ptr && ptr != closer; ptr = ptr->next)
        if (strchr(parser->emphchars, ptr->type))
          delim_drop(list, ptr);
      /* shorten or drop opener&closer items: */
      if (opener->len > m) opener->len -= m;
      else delim_drop(list, opener);
      if (closer->len > m) { ptr = closer; ptr->len -= m; ptr->ofs += m; }
      else { ptr = closer->next; delim_drop(list, closer); }
    }
    else { /* none found */
      ptr = closer->next;
      if (!DELIM_CANOPEN(closer)) delim_drop(list, closer);
    }
  }
}

static size_t
process_links(struct delimlist *list, const char *text, size_t pos, size_t size, SpanTree *tree, Parser *parser)
{
  /* text[pos] is the closing bracket
  // look back on stack for `[` or `![` delim
  // - if not found: emit literal `]`
  // - if found but inactive: drop opener from stack, emit `]`
  // - if found and active: scan ahead for inline/reference link/image
  //   - if mismatch: drop opener from stack, emit `]`
  //   - if found: (1) emit link/image, (2) process nested emphasis
  //     in link body, (3) drop opener from stack (4) if link not
  //     image, make all `[` delims before opener inactive
  */
  struct delim *start, *ptr;
  struct span *span;
  struct slice linkslice, titleslice;
  size_t ofs, end, olen, clen;

  /* look back for opening bracket: */
  for (start = list->tail; start; start = start->prev)
    if (start->type == '[' || start->type == '!') break;
  if (!start) return 1;
  if (!DELIM_ISACTIVE(start)) {
    delim_drop(list, start);
    return 1;
  }

  /* look ahead for rest of link/image: */
  ofs = start->ofs + (start->type == '!' ? 1 : 0);
  end = scan_link_tail(text, size, ofs, pos, parser, &linkslice, &titleslice);
  if (!end) {
    delim_drop(list, start);
    return 1;
  }

  /* add to span tree; note closing delim len is href and title! */
  olen = start->type == '!' ? 2 : 1;
  clen = end - pos;
  span = addspan(tree, start->type, start->ofs, end - start->ofs, olen, clen);
  span->href = linkslice;
  span->title = titleslice;

  /* process body inlines; drop unmatched emph delims: */
  process_emphasis(list, start, pos, tree, parser);
  for (ptr = start; ptr && ptr->ofs < pos; ptr = ptr->next)
    if (strchr(parser->emphchars, ptr->type))
      delim_drop(list, ptr);

  /* brackets before link still group, but don't create links: */
  if (start->type == '[') {  /* link, not image: */
    for (ptr = list->head; ptr && ptr->ofs < pos; ptr = ptr->next)
      if (ptr->type == '[')
        ptr->flags &= ~DELIM_ACTIVE;  /* avoid links in links */
  }

  delim_drop(list, start);
  return end - pos;
}

static void
parse_inlines(Blob *out, const char *text, size_t size, Parser *parser)
{
  static const char blank = ' ';
  struct delimlist delims = DELIMLIST_INIT;
  struct delim *ptr;
  SpanTree tree;
  size_t i, j, len;

  /* create span tree, add root */
  initspans(&tree, 0, size, parser);

  /* add all delimiter runs to a doubly linked list */
  for (j = 0; j < size; ) {
    const char *p = strchr(parser->emphchars, text[j]);
    if (p) { char delim = text[j], before, after;
      for (i = j++; j < size && text[j] == delim; j++);
      before = i > 0 ? text[i-1] : blank;
      after = j < size ? text[j] : blank;
      delim_push(&delims, i, j-i, delim, DELIM_FLAGS(before, delim, after));
    }
    else if (text[j] == '[') {
      /* it's an image if preceded by an unescaped '!' */
      bool isimg = j > 0 && text[j-1] == '!' && (j < 2 || text[j-2] != '\\');
      i = isimg ? j-1 : j;
      j += 1;
      delim_push(&delims, i, j-i, text[i], DELIM_ACTIVE);
    }
    else if (text[j] == ']') {
      j += process_links(&delims, text, j, size, &tree, parser);
    }
    else if (text[j] == '`') {
      struct slice codeslice;
      len = scan_codespan(text+j, size-j, &codeslice);
      if (len) { int flags = ((len-codeslice.n)/2) << DELIM_SHIFT; // TODO HACK
        delim_push(&delims, j, len, '`', flags);
        j += len;
      }
      else j += scan_tickrun(text, size);  /* lonely backtick(s) */
    }
    else if (text[j] == '<') { char type;
      if ((len = scan_autolink(text+j, size-j, &type))) {
        assert(type == ':' || type == '@');
        delim_push(&delims, j, len, type, 0);
        j += len;
      }
      else if ((len = scan_tag(text+j, size-j, 0))) {
        delim_push(&delims, j, len, '<', 0);
        j += len;
      }
      else j += 1;  /* lonely angle bracket */
    }
    else if (text[j] == '\\') j += 2;
    else j++;
  }

  /* links and images are already processed, now do emphasis spans: */
  process_emphasis(&delims, 0, size, &tree, parser);

  /* now codespans, autolinks, rawhtml may still remain: */
  for (ptr = delims.head; ptr; ptr = ptr->next) {
    if (ptr->type == ':' || ptr->type == '@' || ptr->type == '<') {
      addspan(&tree, ptr->type, ptr->ofs, ptr->len, 0, 0);
    }
    else if (ptr->type == '`') {
      size_t dlen = ptr->flags >> DELIM_SHIFT;
      addspan(&tree, ptr->type, ptr->ofs, ptr->len, dlen, dlen);
    }
  }

  free_delims(&delims);

//  dump_spans(&tree);
  emit_spans(out, text, &tree, 1, parser);

  freespans(&tree, parser);
}


/* === block type predicates === */


/** skip up to three initial blanks */
static size_t
preblanks(const char *text, size_t size)
{
  size_t j = 0;
  while (j < 4 && j < size && text[j] == ' ') j += 1;
  return j < 4 ? j : 0;
}


/** check for a blank line; return line length or 0 */
static size_t
is_blankline(const char *text, size_t size)
{
  size_t j;
  for (j=0; j < size && text[j] != '\n' && text[j] != '\r'; j++) {
    if (!ISBLANK(text[j])) return 0;
  }
  if (j < size) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  return j;
}


/** check for an atx heading prefix; return prefix length or 0 */
static size_t
is_atxline(const char *text, size_t size, int *plevel)
{
  int level;
  size_t j = preblanks(text, size);
  for (level = 0; j < size && text[j] == '#' && level < 7; j++, level++);
  if (!(1 <= level && level <= 6)) return 0;
  /* an empty heading is allowed: */
  if (j >= size || text[j] == '\n' || text[j] == '\r') {
    if (plevel) *plevel = level;
    return j;
  }
  /* otherwise need at least one blank or tab: */
  if (!ISBLANK(text[j])) return 0;
  for (++j; j < size && ISBLANK(text[j]); j++);
  if (plevel) *plevel = level;
  return j;
}


/** check for a setext underlining; return line length or 0 */
static size_t
is_setextline(const char *text, size_t size, int *plevel)
{
  size_t len, j = preblanks(text, size);
  if (plevel) *plevel = 0;
  if (j < size && text[j] == '=') {
    for (++j; j < size && text[j] == '='; j++);
    len = is_blankline(text+j, size-j);
    if (!len) return 0;
    if (plevel) *plevel = 1;
    return j+len;
  }
  if (j < size && text[j] == '-') {
    for (++j; j < size && text[j] == '-'; j++);
    len = is_blankline(text+j, size-j);
    if (!len) return 0;
    if (plevel) *plevel = 2;
    return j+len;
  }
  return 0;
}


/** check for blockquote prefix; return prefix length or 0 */
static size_t
is_quoteline(const char *text, size_t size)
{
  size_t j = preblanks(text, size);
  if (j >= size || text[j] != '>') return 0;
  j += 1;
  if (j < size && ISBLANK(text[j])) j += 1;
  return j;
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


/** check for fenced code block; return #ticks or 0 */
static size_t
is_fenceline(const char *text, size_t size)
{
  size_t pre, j;
  char delim;
  pre = preblanks(text, size);
  if (pre >= size) return 0;
  j = pre;
  delim = text[j];
  if (delim != '`' && delim != '~') return 0;
  for (++j; j < size && text[j] == delim; j++);
  if (j - pre < 3) return 0;  /* need at least 3 ticks */
  /* if started by ticks: no more ticks on this line: */
  if (delim == '`') {
    while (j < size && text[j] != '\n' && text[j] != '\r') {
      if (text[j] == delim) return 0;
      j++;
    }
  }
  return j - pre;
}


/** check for hrule line; return length (incl newline) or 0 */
static size_t
is_ruleline(const char *text, size_t size)
{
  size_t j, num = 0;
  char c;

  j = preblanks(text, size);
  /* need at least 3 *|-|_ with optional blanks and nothing else */
  if (j+3 >= size) return 0;  /* too short for an hrule */
  c = text[j];
  if (c != '*' && c != '-' && c != '_') return 0;
  while (j < size && text[j] != '\n' && text[j] != '\r') {
    if (text[j] == c) num += 1;
    else if (!ISBLANK(text[j])) return 0;
    j += 1;
  }
  if (num < 3) return 0;
  if (j < size) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  return j;
}


/** check for an (ordered or unordered) list item line; return prefix length */
static size_t
is_itemline(const char *text, size_t size, char *ptype, int *pstart)
{
  size_t i, j;
  if (ptype) *ptype = '?';
  if (pstart) *pstart = 0;
  j = preblanks(text, size);
  if (j >= size) return 0;
  /* list marker (bullet or ordered) and 1 to 4 blanks
     (if more then 4 blanks: indented code in item); or
     list marker (bullet or ordered) and blank line */
  if (text[j] == '*' || text[j] == '+' || text[j] == '-') {
    if (ptype) *ptype = text[j];
    if (pstart) *pstart = 1;
    j += 1;
    goto post;
  }
  if ('0' <= text[j] && text[j] <= '9') {
    for (i=j++; '0' <= text[j] && text[j] <= '9'; j++);
    if (j-i > 9) return 0;  /* too many digits by CM */
    if (j+1 >= size) return 0;
    if (text[j] != '.' && text[j] != ')') return 0;
    if (ptype) *ptype = text[j];
    if (pstart) *pstart = atoi(text+i);
    j += 1;
    goto post;
  }
  return 0;
post:
  /* now expect 1 to 4 blanks (if more than 4: pretend it's
     just one as we have indented code in list item) -or-
     a blank line, in which case pre is defined to be 2 */
  if (j >= size || text[j] == '\n' || text[j] == '\r' || text[j] == '\t') return j;
  if (text[j] != ' ') return 0;
  if (++j < size && text[j] == ' ')
    if (++j < size && text[j] == ' ')
      if (++j < size && text[j] == ' ')
        if (++j < size && text[j] == ' ')
          j -= 3;  /* indented code */
  return j;
}


/** check for link definition; return length or 0 */
static size_t
is_linkdef(const char *text, size_t size, Linkdef *pdef)
{
  size_t idofs, idend;
  struct slice link, title;
  size_t len, j = preblanks(text, size);

  len = scan_link_label(text+j, size-j);
  if (!len) return 0;
  /* must contain at lest one non-space-char: */
  if (len <= 2 || scan_space(text+j+1, len-2) == len-2) return 0;
  idofs = j+1;
  j += len;
  idend = j-1;

  /* colon must immediately follow the bracket: */
  if (j >= size || text[j] != ':') return 0;
  /* any amount of optional space, but at most one newline: */
  for (++j; j < size && ISBLANK(text[j]); j++);
  if (j >= size) return 0;
  if (text[j] == '\n' || text[j] == '\r') j++;
  if (j >= size) return 0;
  if (text[j] == '\n' && text[j-1] == '\r') j++;
  while (j < size && ISBLANK(text[j])) j++;

  len = scan_link_and_title(text+j, size-j, &link, &title);
  if (!len) return 0;
  j += len;
  len = is_blankline(text+j, size-j);
  if (!len) return 0;  /* junk after link def */
  j += len;

  if (pdef) {
    pdef->id = slice(text+idofs, idend-idofs);
    pdef->link = link;
    pdef->title = title;
  }
  return j;
}


/* === block parsing === */


typedef struct {
  bool unwrapped;
  bool is_block_first;
  bool is_block_last;
} BlockInfo;


static void
parse_blocks(Blob *out, const char *text, size_t size, Parser *parser, BlockInfo *pinfo);


/** parse atx heading; return #chars scanned if found, else 0 */
static size_t
parse_atxheading(Blob *out, const char *text, size_t size, Parser *parser)
{
  int level;
  size_t start, end, len;
  size_t j = is_atxline(text, size, &level);
  if (!j) return 0;
  start = j;
  j += scan_line(text+j, size-j);
  len = j;
  /* scan back over blanks, hashes, and blanks again: */
  while (j > start && ISSPACE(text[j-1])) j--;
  end = j;
  while (j > start && text[j-1] == '#') j--;
  if (j > start && ISBLANK(text[j-1])) {
    for (--j; j > start && ISBLANK(text[j-1]); j--);
    end = j;
  }
  else if (j == start) end = j;

  if (parser->render.heading) {
    Blob *title = blob_get(parser);
    parse_inlines(title, text+start, end-start, parser);
    parser->render.heading(out, level, title, parser->udata);
    blob_put(parser, title);
  }
  return len;
}


static size_t
parse_blockquote(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t i, j, len;
  Blob *temp = blob_get(parser);
  bool wasblank = false;

  for (j = 0; j < size; ) {
    len = is_quoteline(text+j, size-j);
    if (len) { j += len; }
    else if (wasblank ||  /* blank line ends quote, even if quoted */
      is_blankline(text+j, size-j) ||
      is_fenceline(text+j, size-j) ||
      is_codeline(text+j, size-j) ||
      is_ruleline(text+j, size-j) ||
      is_itemline(text+j, size-j, 0, 0)) break;
    /* else: lazy continuation */
    i = j;
    wasblank = is_blankline(text+i, size-i) > 0;
    while (j < size && text[j] != '\n' && text[j] != '\n') j++;
    if (j > i) blob_addbuf(temp, text+i, j-i);
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    blob_addchar(temp, '\n');
  }

  if (too_deep(parser)) { /* do not parse, just render as text */ }
  else {
    Blob *inner = blob_get(parser);
    parse_blocks(inner, blob_str(temp), blob_len(temp), parser, 0);
    blob_put(parser, temp);
    temp = inner;
  }

  if (parser->render.blockquote)
    parser->render.blockquote(out, temp, parser->udata);
  blob_put(parser, temp);
  return j;
}


static size_t
parse_codeblock(Blob *out, const char *text, size_t size, Parser *parser)
{
  static const char *nolang = "";
  size_t i, j, pre, len, mark = 0;
  Blob *temp = blob_get(parser);

  for (j = 0; j < size; ) {
    len = is_blankline(text+j, size-j);
    pre = is_codeline(text+j, size-j);
    if (!pre && !len) break;  /* not indented, not blank: end code block */
    if (len) {
      if (mark > 0) {
        // TODO assumes tabs have been converted to blanks:
        for (pre=0; pre < len && pre < 4 && text[j+pre] == ' '; pre++);
        blob_addbuf(temp, text+j+pre, len-pre);
      }
      j += len;
      continue;
    }
    assert(pre && !len);
    i = j += pre;
    while (j < size && text[j] != '\n' && text[j] != '\r') j++;
    if (j > i) blob_addbuf(temp, text+i, j-i);
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    blob_addchar(temp, '\n');
    mark = blob_len(temp);
  }

  blob_trunc(temp, mark);
  if (parser->render.codeblock)
    parser->render.codeblock(out, nolang, temp, parser->udata);
  blob_put(parser, temp);
  return j;
}


static size_t
parse_fencedcode(Blob *out, const char *text, size_t size, Parser *parser)
{
  /* Inline code and fenced code block are very similar, but not the same:
  // - inline code: n ticks, code, n ticks; n >= 1;
  // - fence block: n ticks, code, n+ ticks, nl; n >= 3; info string
  // In both cases, trim the code.
  */
  Blob *temp;
  size_t pre, j;
  size_t nopen, nclose;
  size_t infofs, infend;
  char delim;

  pre = preblanks(text, size);
  if (pre >= size) return 0;

  j = pre;
  delim = text[j];
  for (++j; j < size && text[j] == delim; j++);
  nopen = j - pre;

  /* line may has info string, but must not have any more ticks: */
  while (j < size && ISBLANK(text[j])) j++;
  infofs = j;
  for (; j < size && text[j] != '\n' && text[j] != '\r'; j++) {
    if (text[j] == delim && delim == '`') return 0;
  }
  infend = j;
  while (infend > infofs && ISBLANK(text[infend-1])) infend--;
  if (j < size) j++;
  if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;

  temp = blob_get(parser);

  while (j < size) {
    size_t start = j, k, end, n;
    while (j < size && text[j] != '\n' && text[j] != '\r') j++;
    end = j;
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    /* look for closing delimiters: */
    n = preblanks(text+start, end-start);
    for (nclose = 0, k = start+n; k < end && text[k] == delim; k++, nclose++);
    if (nclose >= nopen) {
      while (k < size && ISBLANK(text[k])) k++;
      if (k >= size || text[k] == '\n' || text[k] == '\r')
        break;  /* fenced block ended */
    }
    /* remove indent blanks, if any: */
    if (pre) {
      for (k = 0; k < pre && text[start+k] == ' '; k++);
      start += k;
    }

    blob_addbuf(temp, text+start, end-start);
    blob_addchar(temp, '\n');
  }

  if (parser->render.codeblock) {
    Blob *info = blob_get(parser);
    emit_text(info, text+infofs, infend-infofs, parser);
    parser->render.codeblock(out, blob_str(info), temp, parser->udata);
    blob_put(parser, info);
  }

  blob_put(parser, temp);
  return j;
}


static void
do_hrule(Blob *out, Parser *parser)
{
  if (parser->render.hrule)
    parser->render.hrule(out, parser->udata);
}


static size_t
parse_listitem(Blob *out, char type, bool *ploose, const char *text, size_t size, Parser *parser)
{
  Blob *temp;
  size_t pre, i, j, len, sub;
  int start, wasblank;
  char itemtype;
  bool firstblank, nested;

  assert(ploose != 0);

  /* scan item prefix, remember indent: */
  pre = is_itemline(text, size, &itemtype, &start);
  if (!pre || itemtype != type) return 0;  /* list ends */
  if (is_ruleline(text, size)) return 0;  /* lines like "- - -" are hrules */

  j = pre;
  sub = 0;
  nested = false;
  firstblank = false;
  wasblank = 0;
  temp = blob_get(parser);

  /* scan remainder of first line; special case if this line is blank: */
  len = 0;
  if (j >= size || (len = is_blankline(text+j, size-j))) {
    firstblank = true;
    pre = 2;
    j += len;
  }
  else {
    for (i=j; j < size && text[j] != '\n' && text[j] != '\r'; j++);
    blob_addbuf(temp, text+i, j-i);
    blob_addchar(temp, '\n');
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
  }

  /* scan remaining lines, if any: */
  while (j < size) {
    if ((len = is_blankline(text+j, size-j))) {
      j += len;
      if (firstblank) { *ploose = true; break; }
      wasblank += 1;
      continue;
    }
    /* find indent (up to pre): */
    for (i = 0; i < pre && j+i < size && text[j+i] == ' '; i++);
    if (i < pre && is_ruleline(text+j, size-j)) break;
    if (i < 4 && (sub = is_itemline(text+j+i, size-j-i, 0, 0))) {
      if (i >= pre) nested = true;
      if (wasblank && !nested) *ploose = true;
      if (i < pre) break;  /* next (non-sub-) item ends current item */
    }
    if (wasblank) {
      if (i < pre) break;  /* less indented after blank line ends item */
      if (!nested) *ploose = true;
      if (i == pre) nested = false;
    }
    for (; wasblank > 0; wasblank--)
      blob_addchar(temp, '\n');

    /* append stuff after indent */
    j += MIN(i, pre);
    for (i = j; j < size && text[j] != '\n' && text[j] != '\r'; j++);
    blob_addbuf(temp, text+i, j-i);
    blob_addchar(temp, '\n');
    if (j < size) j++;
    if (j < size && text[j-1] == '\r' && text[j] == '\n') j++;
  }

  /* item is now in temp blob; process blocks and inlines: */
  BlockInfo info;
  info.unwrapped = !*ploose;
  Blob *inner = blob_get(parser);
  parse_blocks(inner, blob_str(temp), blob_len(temp), parser, &info);

  if (parser->render.listitem) {
    int tightstart = !info.is_block_first;
    int tightend = !info.is_block_last;
    parser->render.listitem(out, tightstart, tightend, inner, parser->udata);
  }

  blob_put(parser, inner);
  blob_put(parser, temp);

  return j;
}


static size_t
parse_list(Blob *out, char type, int start, const char *text, size_t size, Parser *parser)
{
  Blob *temp = blob_get(parser);
  size_t j, len;
  bool loose = false;

  // TODO collect items into individual blobs and only at the end,
  //      when loose/tight is known, finalize items and emit list

  /* list is sequence of list items of the same type: */
  for (j = 0; j < size; j += len) {
    len = parse_listitem(temp, type, &loose, text+j, size-j, parser);
    if (!len) break;
  }

  if (parser->render.list)
    parser->render.list(out, type, start, temp, parser->udata);
  blob_put(parser, temp);
  return j;
}


static size_t
is_htmlline(const char *text, size_t size, int *pkind, Parser *parser)
{
  /* CM 4.6 distinguishes these kinds of HTML blocks:
  // 1. <pre, <script, <style, or <textarea ... until closing tag
  // 2. <!-- .................................. until -->
  // 3. <? .................................... until ?>
  // 4. <!LETTER .............................. until >
  // 5. <![CDATA[ ............................. until ]]>
  // 6. < or </ followed by one of (case-insensitive) address, article,
  //    aside, base, basefont, blockquote, ... ul (CM 4.6), followed
  //    by a space, a tab, the end of the line, the string ">", or the
  //    string "/>" ........................... until a blank line
  // 7. a complete open tag (but not one from case 1), until blank line
  */
  static const bool oneline = true;
  const struct tagname *name;
  size_t len, i, j = preblanks(text, size);
  bool isclose;

  if (pkind) *pkind = 0;
  if (j+1 >= size) return 0;
  if (text[j] != '<') return 0;
  if (text[j+1] == '?') {
    if (pkind) *pkind = 3;  /* processing instruction */
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
    if (istext && j < size && (text[j] == '>' || ISSPACE(text[j]))) {
      if (pkind) *pkind = isclose ? 7 : 1;
      return j+1;
    }
    else if (j < size && (text[j] == '>' || ISSPACE(text[j]) ||
                         (j+1 < size && text[j] == '/' && text[j+1] == '>'))) {
      if (pkind) *pkind = 6;
      return text[j] == '/' ? j+2 : j+1;
    }
    return 0;
  }
  if ((len = scan_tag(text+i, size-i, oneline)) &&
      is_blankline(text+i+len, size-i-len)) {
    if (pkind) *pkind = 7;
    return i + len;
  }
  return 0;
}


static size_t
parse_htmlblock(Blob *out, const char *text, size_t size, size_t startlen, int kind, Parser *parser)
{
  /* Assume text points to "start condition" of the given kind (CM 4.6)
  // and startlen is the length of this start condition. End condition is:
  // kind=1: </pre> or </script> or </style> or </textarea>
  // kind=2:    -->
  // kind=3:     ?>       For kinds 1..5 scan to '>' and
  // kind=4:      >       look back for the specifics
  // kind=5:    ]]>
  // kind=6,7: blank line
  */
  size_t j = startlen;

  if (kind == 6 || kind == 7) {
    while (j < size) {
      size_t len = scan_line(text+j, size-j);
      j += len;
      if (is_blankline(text+j, size-j)) break;
    }
    blob_addbuf(out, text, j);
    return j;
  }

  while (j < size) {
    const char *p = memchr(text+j, '>', size-j);
    if (!p) { j = size; break; }  /* not found: to end of block or file */
    j = p - text + 1;
    if ((kind == 2 && p >= text+ 6 && p[-2] == '-' && p[-1] == '-') ||
        (kind == 3 && p >= text+ 5 &&                 p[-1] == '?') ||
        (kind == 4)                                                 ||
        (kind == 5 && p >= text+11 && p[-2] == ']' && p[-1] == ']')) {
      break;  /* end condition found */
    }
    else if (kind == 1) {
      if (p >= text+10 && !strnicmp("</pre", p-5, 5)) break;
      if (p >= text+16 && !strnicmp("</script", p-8, 8)) break;
      if (p >= text+14 && !strnicmp("</style", p-7, 7)) break;
      if (p <= text+20 && !strnicmp("</textarea", p-10, 10)) break;
    }
  }

  /* scan to end of line (cf CM 4.6): */
  j += scan_line(text+j, size-j);

  if (parser->render.htmlblock) {
    Blob html = { (char *) text, j, 0 };  /* static blob */
    parser->render.htmlblock(out, &html, parser->udata);
  }

  return j;
}


static size_t
parse_paragraph(Blob *out, const char *text, size_t size, Parser *parser, bool *punwrapped)
{
  Blob *temp = blob_get(parser);
  const char *s;
  size_t j, k, len, n;
  int level, kind, start;
  bool unwrapped = punwrapped && *punwrapped;

  for (j = n = 0, level = 0; j < size; j += len) {
    len = scan_line(text+j, size-j);
    if (!len) break;
    if (is_blankline(text+j, len)) break;
    if (is_atxline(text+j, len, 0)) break;
    if (j > 0 && (n=is_setextline(text+j, len, &level))) break;  /* not 1st line */
    if (is_ruleline(text+j, len)) break;
    if (is_fenceline(text+j, len)) break;
    if ((n = is_itemline(text+j, len, 0, &start)) && start == 1 &&
        !is_blankline(text+j+n, len-n)) break;
    if (is_quoteline(text+j, len)) break;
    if (is_htmlline(text+j, len, &kind, parser) && kind != 7) break;
    for (k = 0; k < len && ISBLANK(text[j+k]); k++);
    blob_addbuf(temp, text+j+k, len-k);
    /* NOTE trailing blanks and breaks handled by inline parsing */
  }

  s = blob_str(temp);
  k = blob_len(temp);
  while (k > 0 && (s[k-1] == '\n' || s[k-1] == '\r')) k--;
  blob_trunc(temp, k);

  if (level > 0 && blob_len(temp) > 0) {
    if (parser->render.heading) {
      Blob *title = blob_get(parser);
      blob_trimend(temp);
      parse_inlines(title, blob_str(temp), blob_len(temp), parser);
      parser->render.heading(out, level, title, parser->udata);
      blob_put(parser, title);
      if (punwrapped) *punwrapped = false;
    }
    j += n;  /* consume the setext underlining */
  }
  else if (j > 0) {
    if (!unwrapped && parser->render.paragraph) {
      Blob *para = blob_get(parser);
      parse_inlines(para, blob_str(temp), blob_len(temp), parser);
      parser->render.paragraph(out, para, parser->udata);
      blob_put(parser, para);
    }
    else {
      parse_inlines(out, blob_str(temp), blob_len(temp), parser);
    }
  }

  blob_put(parser, temp);
  return j;
}


static void
parse_blocks(Blob *out, const char *text, size_t size, Parser *parser, BlockInfo *pinfo)
{
  size_t start;
  char itemtype;
  int itemstart;
  int htmlkind;
  bool unwrapped = pinfo && pinfo->unwrapped;
  bool block1 = 0, blockN = 0;

  parser->nesting_depth++;
  for (start=0; start<size; ) {
    const char *ptr = text+start;
    size_t len, end = size-start;
    bool isblock = true;

    if ((len = parse_atxheading(out, ptr, end, parser))) {
      /* nothing else to do */
    }
    else if (is_codeline(ptr, end)) {
      len = parse_codeblock(out, ptr, end, parser);
    }
    else if (is_quoteline(ptr, end)) {
      len = parse_blockquote(out, ptr, end, parser);
    }
    else if ((len = is_ruleline(ptr, end))) {
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
      isblock = false;
    }
    else if ((len = is_blankline(ptr, end))) {
      /* nothing to do, blank lines separate blocks */
      isblock = false;
    }
    else {
      /* Non-blank lines that cannot be interpreted otherwise
         form a paragraph in Markdown/CommonMark: */
      len = parse_paragraph(out, ptr, end, parser, &unwrapped);
      isblock = !unwrapped;
    }

    if (start == 0) block1 = isblock;
    blockN = isblock;

    if (len) start += len;
    else break;
  }
  parser->nesting_depth--;

  if (pinfo) {
    pinfo->is_block_first = block1;
    pinfo->is_block_last = blockN;
  }
}


static void
init(Parser *parser, struct markdown *mkdn, int debug)
{
  assert(parser != 0);
  assert(mkdn != 0);

  parser->render = *mkdn;
  parser->udata = mkdn->udata;
  parser->debug = debug < 1 ? 0 : debug;
  parser->nesting_depth = 0;
  parser->in_link = 0;
  parser->linkdefs = (Blob) BLOB_INIT;

  memset(parser->blob_pool, 0, sizeof(parser->blob_pool));
  parser->pool_index = 0;

  memset(parser->emphchars, 0, sizeof(parser->emphchars));
  if (mkdn->emphasis) {
    char *t = parser->emphchars;
    const char *end = t + sizeof(parser->emphchars) - 1;
    const char *s =  mkdn->emphchars;
    assert(t+2 < end);
    if (!s) s = "*_";  /* Markdown's standard emphasis delimiters */
    for (; *s && t < end; s++, t++) {
      if (ISASCII(*s) && ISPUNCT(*s)) *t = *s;
    }
  }

  memset(parser->livechars, 0, sizeof(parser->livechars));
  parser->livechars['\\'] = emit_escape;
  if (mkdn->entity) parser->livechars['&'] = emit_entity;
  if (mkdn->linebreak) parser->livechars['\n'] = emit_linebreak;
  if (mkdn->linebreak) parser->livechars['\r'] = emit_linebreak;

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
markdown(Blob *out, const char *text, size_t size, struct markdown *mkdn, int debug)
{
  size_t start;
  Parser parser;

  if (!text || !size || !mkdn) return;
  assert(out != NULL);

  init(&parser, mkdn, debug);

  /* 1st pass: collect references */
  // TODO too simplistic for CM: must see linkdefs in block context
  for (start = 0; start < size; ) {
    Linkdef linkdef;
    size_t len = is_linkdef(text+start, size-start, &linkdef);
    if (len) {
      start += len;
      blob_addbuf(&parser.linkdefs, (void*)&linkdef, sizeof(linkdef));
    }
    else {
      // TODO pre process into buffer: expand tabs (size=4), newlines to \n, \0 to U+FFFD
      len = scan_line(text+start, size-start);
      if (len) start += len;
      else break;
    }
  }

  /* sort link defs to allow binary search later on */
  if (blob_len(&parser.linkdefs) > 0) {
    qsort(blob_buf(&parser.linkdefs),
          blob_len(&parser.linkdefs)/sizeof(struct linkdef),
          sizeof(struct linkdef), linkdef_cmp);
  }

  /* 2nd pass: do the rendering */
  if (mkdn->prolog) mkdn->prolog(out, mkdn->udata);
  parse_blocks(out, text, size, &parser, false);
  if (mkdn->epilog) mkdn->epilog(out, mkdn->udata);

  /* release memory */
  assert(parser.nesting_depth == 0);
  blob_free(&parser.linkdefs);
  while (parser.pool_index > 0) {
    parser.pool_index--;
    blob_free(parser.blob_pool[parser.pool_index]);
    mem_free(parser.blob_pool[parser.pool_index]);
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
  const int debug = 1;

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

  mkdnhtml(&output, blob_str(&input), blob_len(&input), 0, 256, debug);

  fputs(blob_str(&output), stdout);
  fflush(stdout);

  blob_free(&input);
  blob_free(&output);

  return 0;
}

#endif
