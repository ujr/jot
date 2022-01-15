
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
 * rendering in a second pass over the Markdown. The second
 * pass will parse the input into a sequence of "blocks"
 * and, within each block, parse "inlines" such as emphasis
 * or links. Some block types can contain other blocks.
 *
 * The parser here is inspired by the one in Fossil SCM,
 * which itself derives from the parser by Natacha Porté.
 */


#define ISASCII(c) (0 <= (c) && (c) <= 127)
#define ISCNTRL(c) ((c) < 32 || (c) == 127)
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

#define UNUSED(x) ((void)(x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* for meaningful assertions */
#define OUT_OF_MEMORY 0
#define INVALID_HTMLBLOCK_KIND 0
#define NOT_REACHED 0

/* length of array (number of items) */
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
scan_tag(const char *text, size_t size)
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
  while (len < size && ISBLANK(text[len])) len++;
  if (len >= size) return 0;
  if (text[len] == '>') return len+1;
  if (closing) return 0; /* closing tag cannot have attrs */
  if (text[len] == '/') {
    len += 1;
    return len < size && text[len] == '>' ? len+1 : 0;
  }
  if (!ISBLANK(text[len-1])) return 0;  /* attrs must be separated */
  /* scan over (possibly quoted) attributes: */
  quote = 0;
  while (len < size && (c=text[len]) != 0 && (c != '>' || quote)) {
    if (c == quote) quote = 0;
    else if (!quote && (c == '"' || c == '\'')) quote = c;
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

  static const char *extra = "@.-+_=.~";  /* allowed in mail beyond alnum */
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
  if (ptype) *ptype = 'L';  /* hyperlink */
  return j+1;

trymail:
  /* scan over "reasonable" characters until '>'; mail if exactly one '@': */
  for (nat = 0; j < size && (ISALNUM(text[j]) || strchr(extra, text[j])); j++) {
    if (text[j] == '@') nat++;
  }
  if (j >= size || text[j] != '>' || nat != 1) return 0;
  if (ptype) *ptype = mailto ? 'M' : '@';
  return j+1;
}


/** scan link label like [foo]; return length or zero */
static size_t
scan_link_label(const char *text, size_t size)
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
  /* Link types:
     - inline:    [text](dest "title")
     - reference: [text][ref]
     - collapsed: [text][]     let ref = text
     - shortcut:  [text]       let ref = text
  */
  const char *bodyptr;
  size_t j = 0, len, bodylen;

  if (size < 2 || text[j] != '[') return 0;
  len = scan_link_body(text, size);
  if (!len) return 0;
  j += len;

  bodyptr = text+1;
  bodylen = len-2;
  if (bodyslice) *bodyslice = slice(bodyptr, bodylen);

  /* link must follow immediately (no blanks) */
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


/** scan to next span (emph, strong) char, skipping other spans */
static size_t
scan_span(const char *text, size_t size, char c)
{
  size_t j = 0, len;
  while (j < size) {
    while (j < size && text[j] != c && text[j] != '[' && text[j] != '`') j++;
    if (j >= size) return 0;
    if (j && text[j-1] == '\\') { j++; continue; }  /* escaped */
    if (text[j] == c) return j;  /* found */
    if (text[j] == '`') {  /* skip codespan */
      len = scan_codespan(text+j, size-j, 0);
      if (len) j += len; else j += 1;  /* span or lonely backtick */
      continue;
    }
    if (text[j] == '[') {  /* skip link or image */
      len = scan_full_link(text+j, size-j, 0, 0, 0, 0);
      if (len) j += len; else j += 1;  /* link or lonely bracket */
      continue;
    }
    assert(NOT_REACHED);
  }
  return 0;
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


static bool
too_deep(Parser *parser)
{
  return parser->nesting_depth > 100;
}


static void
parse_inlines(Blob *out, const char *text, size_t size, Parser *parser)
{
  SpanProc action;
  size_t i, j, n;
  /* copy chars to output, looking for "active" chars */
  i = j = 0;
  for (;;) {
    for (; j < size; j++) {
      unsigned uc = ((unsigned) text[j]) & 0x7F;
      action = parser->active_chars[uc];
      if (action && !too_deep(parser)) break;
    }
    if (j > i) {
      if (parser->render.text)
        parser->render.text(out, text+i, j-i, parser->udata);
      else blob_addbuf(out, text+i, j-i);
      i = j;
    }
    if (j >= size) break;
    parser->nesting_depth++;
    assert(action != 0);
    n = action(out, text+j, size-j, j, parser);
    parser->nesting_depth--;
    if (n) { j += n; i = j; } else j += 1;
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
#define DELIM_OPENER 2
#define DELIM_CLOSER 4

static int
can_open(char before, char after)
{
  if (ISSPACE(after)) return 0;
  if (ISALNUM(after)) return DELIM_OPENER;
  if (ISALNUM(before)) return 0;
  return DELIM_OPENER;
  // TODO special: if c==_ and ISALNUM(before) return false
}

static bool
can_close(char before, char after)
{
  if (ISSPACE(before)) return 0;
  if (ISALNUM(before)) return DELIM_CLOSER;
  if (ISALNUM(after)) return 0;
  return DELIM_CLOSER;
}


static size_t
emphasis1(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t j, len, ok;
  char before, after;
  char c = text[0];  /* guaranteed by caller */

  for (j = 1; j < size; ) {
    len = scan_span(text+j, size-j, c);
    if (!len) return 0;
    j += len;
    if (j >= size) return 0;
    assert(text[j] == c);
    if (j+1 < size && text[j+1] == c) {
      j += 2;  /* skip double delims */
      continue;
    }
    before = text[j-1];
    after = j+1 < size ? text[j+1] : ' ';
    if (j < size && text[j] == c &&
        can_close(before, after) && (c != '_' || !ISALNUM(after))) {
      Blob *temp = pool_get(parser);
      parse_inlines(temp, text+1, j-1, parser);
      ok = parser->render.emphasis(out, c, 1, temp, parser->udata);
      pool_put(parser, temp);
      return ok ? j+1 : 0;
    }
  }
  return 0;
}

static size_t
emphasis2(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t j, len, ok;
  char before, after;
  char c = text[0];

  for (j = 2; j < size; ) {
    len = scan_span(text+j, size-j, c);
    if (!len) return 0;
    j += len;
    if (j >= size) return 0;

    before = text[j-1];
    assert(text[j] == c);
    j += 1;
    if (j >= size || text[j] != c) continue;  /* not double delim */
    j += 1;
    after = j < size ? text[j] : ' ';
    if (can_close(before, after) && (c != '_' || !ISALNUM(after))) {
      Blob *temp = pool_get(parser);
      parse_inlines(temp, text+2, j-4, parser);
      ok = parser->render.emphasis(out, c, 2, temp, parser->udata);
      return ok ? j+2 : 0;
    }
  }
  return 0;
}

static size_t
do_emphasis(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  char c, before;
  UNUSED(prefix);
  if (!size) return 0;
  c = text[0];
  before = prefix > 0 ? text[-1] : ' ';

  if (size > 1 && text[1] != c) {
    if (!can_open(before, text[1])) return 0;
    if (c == '_' && ISALNUM(before)) return 0;
    return emphasis1(out, text, size, parser);
  }

  if (size > 2 && text[1] == c && text[2] != c) {
    if (!can_open(before, text[2])) return 0;
    if (c == '_' && ISALNUM(before)) return 0;
    return emphasis2(out, text, size, parser);
  }

  // TODO triple emphasis

  return 0;
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
    j += 1;  /* leave i unchanged to add \ to run of text */
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


static size_t
do_codespan(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  Blob *body;
  struct slice codeslice;
  size_t len;
  int ok;
  UNUSED(prefix);

  len = scan_codespan(text, size, &codeslice);
  if (!len) {  /* emit tick run (override parse_inline's one char default) */
    len = scan_tickrun(text, size);
    if (parser->render.text)
        parser->render.text(out, text, len, parser->udata);
    else blob_addbuf(out, text, len);
    return len;
  }

  body = pool_get(parser);
  /* by CM 6.1, convert newlines to blanks: */
  emit_codespan(body, codeslice.s, codeslice.n);
  assert(parser->render.codespan != 0);
  ok = parser->render.codespan(out, body, parser->udata) ? len : 0;
  pool_put(parser, body);
  return ok ? len : 0;
}


static size_t
do_anglebrack(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  /* autolink -or- raw html tag; see CM 6.5 and 6.6 */
  char type;
  size_t len;
  UNUSED(prefix);
  len = scan_tag(text, size);
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


static size_t
do_link(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  Blob *body, *link, *title;
  struct slice textslice, linkslice, titleslice;
  size_t len;
  int ok;
  UNUSED(prefix);

  if (parser->in_link > 0) return 0;
  len = scan_full_link(text, size, parser, &textslice, &linkslice, &titleslice);
  if (!len) return 0;

  body = pool_get(parser);
  link = pool_get(parser);
  title = pool_get(parser);

  /* parse link text, but avoid links within links: */
  parser->in_link++;
  parse_inlines(body, textslice.s, textslice.n, parser);
  do_escapes(link, linkslice.s, linkslice.n);
  do_escapes(title, titleslice.s, titleslice.n);
  parser->in_link--;

  assert(parser->render.link != 0);
  ok = parser->render.link(out, link, title, body, parser->udata);

  pool_put(parser, title);
  pool_put(parser, link);
  pool_put(parser, body);
  return ok ? len : 0;
}


static size_t
do_image(
  Blob *out, const char *text, size_t size, size_t prefix, Parser *parser)
{
  Blob *body, *link, *title;
  struct slice textslice, linkslice, titleslice;
  size_t len;
  int ok;
  UNUSED(prefix);

  if (size < 3 || text[1] != '[') return 0;
  len = scan_full_link(text+1, size-1, parser, &textslice, &linkslice, &titleslice);
  if (!len) return 0;

  body = pool_get(parser);
  link = pool_get(parser);
  title = pool_get(parser);

  /* do not parse body: it merely becomes the img alt attribute: */
  blob_addbuf(body, textslice.s, textslice.n);
  do_escapes(link, linkslice.s, linkslice.n);
  do_escapes(title, titleslice.s, titleslice.n);

  assert(parser->render.image != 0);
  ok = parser->render.image(out, link, title, body, parser->udata);

  pool_put(parser, title);
  pool_put(parser, link);
  pool_put(parser, body);
  return ok ? 1+len : 0;
}


/** block parsing **/


static void
parse_blocks(Blob *out, const char *text, size_t size, Parser *parser);


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


/** link definition like: [label]: link "title" */
static size_t
is_linkdef(const char *text, size_t size, Linkdef *pdef)
{
  size_t idofs, idend;
  struct slice link, title;
  size_t len, j = preblanks(text, size);

  len = scan_link_label(text+j, size-j);
  if (!len) return 0;
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
  size_t i, j, len, blankrun = 0;
  Blob *temp = pool_get(parser);

  for (j = 0; j < size; ) {
    len = is_codeline(text+j, size-j);
    if (len) { j += len;
      if (blankrun) blob_addchar(temp, '\n');
      blankrun = 0;
    }
    else {
      len = is_blankline(text+j, size-j);
      if (!len) break;  /* not indented, not blank: end code block */
      j += len;
      blankrun += 1;
      continue;
    }
    i = j;
    while (j < size && text[j] != '\n' && text[j] != '\r') j++;
    if (j > i) blob_addbuf(temp, text+i, j-i);
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    blob_addchar(temp, '\n');
  }

  if (parser->render.codeblock)
    parser->render.codeblock(out, nolang, temp, parser->udata);
  pool_put(parser, temp);
  return j;
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


static size_t
parse_blockquote(Blob *out, const char *text, size_t size, Parser *parser)
{
  size_t i, j, len;
  Blob *temp = pool_get(parser);

  for (j = 0; j < size; ) {
    len = is_quoteline(text+j, size-j);
    if (len) { j += len; }
    else {
      len = is_blankline(text+j, size-j);
      if (len && (j+len >= size ||
                  (!is_quoteline(text+j+len, size-j-len) &&
                   !is_blankline(text+j+len, size-j-len))))
        break;  /* empty line followed by non-quote line ends quote */
    }
    i = j;
    while (j < size && text[j] != '\n' && text[j] != '\n') j++;
    if (j > i) blob_addbuf(temp, text+i, j-i);
    if (j < size) j++;
    if (j < size && text[j] == '\n' && text[j-1] == '\r') j++;
    blob_addchar(temp, '\n');
  }

  if (too_deep(parser)) { /* do not parse, just render as text */ }
  else {
    Blob *inner = pool_get(parser);
    parse_blocks(inner, blob_str(temp), blob_len(temp), parser);
    pool_put(parser, temp);
    temp = inner;
  }

  if (parser->render.blockquote)
    parser->render.blockquote(out, temp, parser->udata);
  pool_put(parser, temp);
  return j;
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


static size_t
parse_fencedcode(Blob *out, const char *text, size_t size, Parser *parser)
{
  /* Inline code and fenced code block are very similar, but not the same:
  // - inline code: n ticks, code, n ticks; n >= 1;
  // - fence block: n ticks, code, n+ ticks, nl; n >= 3; info string
  // In both cases, trim the code.
  */
  Blob *temp = pool_get(parser);
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
    Blob *info = pool_get(parser);
    do_escapes(info, text+infofs, infend-infofs);
    parser->render.codeblock(out, blob_str(info), temp, parser->udata);
    pool_put(parser, info);
  }

  pool_put(parser, temp);
  return j;
}


/** check for hrule line; return length (incl newline) or 0 */
static size_t
is_hrule(const char *text, size_t size)
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
  if (!pre || itemtype != type) return 0;  /* list ends */

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
      if (i < pre) break;  /* next item ends current item */
      if (!sublist) sublist = blob_len(temp);  /* sublist start offset */
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
      parse_blocks(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, parser);
    }
    else {
      parse_inlines(inner, blob_buf(temp), blob_len(temp), parser);
    }
  }
  else {
    if (sublist && sublist < blob_len(temp)) {
      /* Want two blocks: the stuff before, and then the sublist */
      parse_blocks(inner, blob_buf(temp), sublist, parser);
      parse_blocks(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, parser);
    }
    else {
      parse_blocks(inner, blob_buf(temp), blob_len(temp), parser);
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
  size_t j, len;
  bool loose = false;

  /* list is sequence of list items of the same type: */
  for (j = 0; j < size; j += len) {
    len = parse_listitem(temp, type, &loose, text+j, size-j, parser);
    if (!len) break;
  }

  if (parser->render.list)
    parser->render.list(out, type, start, temp, parser->udata);
  pool_put(parser, temp);
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
    if (!isclose && istext && j < size && (text[j] == '>' || ISSPACE(text[j]))) {
      if (pkind) *pkind = 1;
      return j+1;
    }
    else if (j < size && (text[j] == '>' || ISSPACE(text[j]) ||
                         (j+1 < size && text[j] == '/' && text[j+1] == '>'))) {
      if (pkind) *pkind = 6;
      return text[j] == '/' ? j+2 : j+1;
    }
    return 0;
  }
  if ((len = scan_tag(text+i, size-i)) &&
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
    else if (kind == 1) { // TODO ignore case!
      if (p >= text+10 && !memcmp("</pre", p-5, 5)) break;
      if (p >= text+16 && !memcmp("</script", p-8, 8)) break;
      if (p >= text+14 && !memcmp("</style", p-7, 7)) break;
      if (p <= text+20 && !memcmp("</textarea", p-10, 10)) break;
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
parse_paragraph(Blob *out, const char *text, size_t size, Parser *parser)
{
  Blob *temp = pool_get(parser);
  const char *s;
  size_t j, k, len, n;
  int level;

  for (j = n = 0, level = 0; j < size; j += len) {
    len = scan_line(text+j, size-j);
    if (!len) break;
    if (is_blankline(text+j, len)) break;
    if (is_atxline(text+j, len, 0)) break;
    if (j > 0 && (n=is_setextline(text+j, len, &level))) break;  /* not 1st line */
    if (is_hrule(text+j, len)) break;
    if (is_fenceline(text+j, len)) break;
    if (is_itemline(text+j, len, 0, 0)) break;
    if (is_htmlline(text+j, len, 0, parser)) break;
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
      Blob *title = pool_get(parser);
      parse_inlines(title, blob_buf(temp), blob_len(temp), parser);
      parser->render.heading(out, level, title, parser->udata);
      pool_put(parser, title);
    }
    j += n;  /* consume the setext underlining */
  }
  else if (j > 0 && parser->render.paragraph) {
    Blob *para = pool_get(parser);
    parse_inlines(para, blob_buf(temp), blob_len(temp), parser);
    parser->render.paragraph(out, para, parser->udata);
    pool_put(parser, para);
  }

  pool_put(parser, temp);
  return j;
}


static void
parse_blocks(Blob *out, const char *text, size_t size, Parser *parser)
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
  if (mkdn->link) parser->active_chars['['] = do_link;
  if (mkdn->image) parser->active_chars['!'] = do_image;
  if (mkdn->emphasis) {
    parser->active_chars['*'] = do_emphasis;
    parser->active_chars['_'] = do_emphasis;
    if (mkdn->spanchars) { const char *p;
      for (p = mkdn->spanchars; *p; p++)
        if (ISASCII(*p) && ISPUNCT(*p))
          parser->active_chars[((unsigned) *p)&0x7F] = do_emphasis;
    }
  }

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
      len = scan_line(text+start, size-start);
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
  parse_blocks(out, text, size, &parser);
  if (mkdn->epilog) mkdn->epilog(out, mkdn->udata);

  /* release memory */
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
