
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "log.h"
#include "mkdn.h"


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


#define ISSPACE(c) ((c) == ' ' || ('\t' <= (c) && (c) <= '\r'))
// the above includes VT and FF, which we may not want?!
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#define ISLOWER(c) ('a' <= (c) && (c) <= 'z')
#define ISUPPER(c) ('A' <= (c) && (c) <= 'Z')
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

#define UNUSED(x) ((void)(x))

/** length of array (number of items) */
#define ARLEN(a) (sizeof(a)/sizeof((a)[0]))


typedef struct slice {
  const char *s;
  size_t n;
} Slice;


typedef struct renderer {
  struct markdown cb;
  void *udata;
  int nesting_depth;
  Blob *blob_pool[32];
  size_t pool_index;
} Renderer;


static Slice slice(const char *s, size_t len) {
  Slice slice = { s, len };
  return slice;
}


static void printslice(Slice slice, FILE *fp)
{
  fwrite(slice.s, 1, slice.n, fp);
}


static bool
too_deep(Renderer *rndr)
{
  return rndr->nesting_depth > 100;
}


static Blob*
pool_get(Renderer *rndr)
{
  static const Blob empty = BLOB_INIT;
  Blob *ptr;
  if (rndr->pool_index > 0)
    return rndr->blob_pool[--rndr->pool_index];
  ptr = malloc(sizeof(*ptr));
  if (!ptr) {
    perror("malloc");
    abort(); // TODO error handler
  }
  *ptr = empty;
  return ptr;
}


static void
pool_put(Renderer *rndr, Blob *blob)
{
  if (!blob) return;
  if (rndr->pool_index < ARLEN(rndr->blob_pool)) {
    blob_clear(blob);
    rndr->blob_pool[rndr->pool_index++] = blob;
  }
  else {
    log_debug("mkdn: blob pool exhausted; freeing blob instead of caching");
    blob_free(blob);
    free(blob);
  }
}


static void
parse_inlines(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  UNUSED(rndr);
  blob_addbuf(out, text, size); // TODO
}


static void
parse_block(Blob *out, const char *text, size_t size, Renderer *rndr);


/** scan past next newline; return #bytes scanned */
static size_t
scanline(const char *text, size_t size) // TODO rename skipline?
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


/** link definition: [label]: link "title" */
static size_t
is_linkdef(const char *text, size_t size)
{
  size_t i = 0, end;
  size_t idofs, idend;
  size_t linkofs, linkend;
  size_t titlofs, titlend;

  // leading space: at most 3 blanks, optional
  if (size < 4) return 0; // too short for a label defn
  while (i < 4 && text[i] == ' ') i += 1;
  if (i >= 4) return 0; // too much indented

  if (text[i] != '[') return 0;
  idofs = ++i;
  while (i < size && text[i] != '\n' && text[i] != '\r' && text[i] != ']') i++;
  if (i >= size || text[i] != ']') return 0;
  idend = i++;

  // colon must immediately follow the bracket
  if (i >= size || text[i] != ':') return 0;
  // any amount of optional space, but at most one newline
  for (++i; i < size && ISBLANK(text[i]); i++);
  if (i >= size) return 0;
  if (text[i] == '\n' || text[i] == '\r') i++;
  if (i >= size) return 0;
  if (text[i] == '\n' && text[i-1] == '\r') i++;
  while (i < size && ISBLANK(text[i])) i++;

  // the link itself may be in angle brackets but contains no space
  if (i < size && text[i] == '<') i++;
  linkofs = i;
  while (i < size && !ISSPACE(text[i]) && text[i] != '>') i++;
  linkend = text[i-1] == '>' ? i-1 : i;

  // optional title, may on next line
  while (i < size && ISBLANK(text[i])) i++;
  if (i < size && (text[i] == '\n' || text[i] == '\r')) i++;
  if (i < size && text[i] == '\n' && text[i-1] == '\r') i++;
  end = i;
  while (i < size && ISBLANK(text[i])) i++;

  titlofs = titlend = 0;
  if (i < size && (text[i] == '"' || text[i] == '\'' || text[i] == '(')) {
    char delim = text[i++];
    titlofs = i;
    while (i < size && text[i] != '\n' && text[i] != '\r') i++;
    end = (i+1 < size && text[i]=='\r' && text[i+1] == '\n') ? i+2 : i+1;
    for (--i; i > titlofs && ISBLANK(text[i]); i--);
    titlend = i;
    if (i == titlofs || text[i] != delim) return 0;
  }

  fprintf(stderr, "Link defn: {");
  printslice(slice(text+idofs, idend-idofs), stderr);
  fprintf(stderr, "|");
  printslice(slice(text+linkofs, linkend-linkofs), stderr);
  fprintf(stderr, "|");
  printslice(slice(text+titlofs, titlend-titlofs), stderr);
  fprintf(stderr, "}\n");

  return end;
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
parse_atxheading(Blob *out, const char *text, size_t size, Renderer *rndr)
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

  if (rndr->cb.heading) {
    Blob *title = pool_get(rndr);
    parse_inlines(title, text+start, end-start, rndr);
    rndr->cb.heading(out, level, title, rndr->udata);
    pool_put(rndr, title);
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
parse_codeblock(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  static const char *nolang = "";
  size_t i, i0, len, blankrun = 0;
  Blob *temp = pool_get(rndr);

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

  if (rndr->cb.codeblock)
    rndr->cb.codeblock(out, nolang, temp, rndr->udata);
  pool_put(rndr, temp);
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
parse_blockquote(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  size_t i, i0, len;
  Blob *temp = pool_get(rndr);

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

  if (!too_deep(rndr)) {
    Blob *inner = pool_get(rndr);
    parse_block(inner, blob_str(temp), blob_len(temp), rndr);
    pool_put(rndr, temp);
    temp = inner;
  }
  // else too deep: do not parse inner, just render as text

  if (rndr->cb.blockquote)
    rndr->cb.blockquote(out, temp, rndr->udata);
  pool_put(rndr, temp);
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
  /* but no more ticks on this line: */
  for (; i < size && text[i] != '\n' && text[i] != '\r'; i++) {
    if (text[i] == delim) return 0;
  }
  return i - pre;
}

// inline code: n ticks, code, n ticks; trim code; n >= 1
// block code: n ticks, code n+ ticks, nl; info string; trim code; n >= 3

static size_t
parse_fencedcode(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  Blob *temp = pool_get(rndr);
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
    if (text[i] == delim) return 0;
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

  if (rndr->cb.codeblock) {
    Blob *info = pool_get(rndr);
    blob_addbuf(info, text+infofs, infend-infofs);
    rndr->cb.codeblock(out, blob_str(info), temp, rndr->udata);
    pool_put(rndr, info);
  }

  pool_put(rndr, temp);
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
do_hrule(Blob *out, Renderer *rndr)
{
  if (rndr->cb.hrule)
    rndr->cb.hrule(out, rndr->udata);
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
parse_listitem(Blob *out, char type, bool *ploose, const char *text, size_t size, Renderer *rndr)
{
  Blob *temp = pool_get(rndr);
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
  Blob *inner = pool_get(rndr);
  if (!*ploose || too_deep(rndr)) {
    if (sublist && sublist < blob_len(temp)) {
      parse_inlines(inner, blob_buf(temp), sublist, rndr);
      parse_block(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, rndr);
    }
    else {
      parse_inlines(inner, blob_buf(temp), blob_len(temp), rndr);
    }
  }
  else {
    if (sublist && sublist < blob_len(temp)) {
      /* Want two blocks: the stuff before, and then the sublist */
      parse_block(inner, blob_buf(temp), sublist, rndr);
      parse_block(inner, blob_buf(temp)+sublist, blob_len(temp)-sublist, rndr);
    }
    else {
      parse_block(inner, blob_buf(temp), blob_len(temp), rndr);
    }
  }

  if (rndr->cb.listitem)
    rndr->cb.listitem(out, inner, rndr->udata);

  pool_put(rndr, inner);
  pool_put(rndr, temp);

  return j;
}


static size_t
parse_list(Blob *out, char type, int start, const char *text, size_t size, Renderer *rndr)
{
  Blob *temp = pool_get(rndr);
  size_t i, len;
  bool loose = false;

  /* list is sequence of list items ofblob the same type: */
  for (i = 0; i < size; ) {
    len = parse_listitem(temp, type, &loose, text+i, size-i, rndr);
    if (!len) break;
    i += len;
  }

  if (rndr->cb.list)
    rndr->cb.list(out, type, start, temp, rndr->udata);
  pool_put(rndr, temp);
  return i;
}


static size_t
parse_paragraph(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  Blob *temp = pool_get(rndr);
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
    // NOTE trailing blanks and hard breaks handled by inline parsing
    i += len;
  }

  s = blob_str(temp);
  j = blob_len(temp);
  while (j > 0 && (s[j-1] == '\n' || s[j-1] == '\r')) j--;
  blob_trunc(temp, j);

  if (level > 0 && blob_len(temp) > 0) {
    if (rndr->cb.heading) {
      Blob *title = pool_get(rndr);
      parse_inlines(title, blob_buf(temp), blob_len(temp), rndr);
      rndr->cb.heading(out, level, title, rndr->udata);
      pool_put(rndr, title);
    }
    i += n; /* consume the setext underlining */
  }
  else if (i > 0 && rndr->cb.paragraph) {
    Blob *para = pool_get(rndr);
    parse_inlines(para, blob_buf(temp), blob_len(temp), rndr);
    rndr->cb.paragraph(out, para, rndr->udata);
    pool_put(rndr, para);
  }

  pool_put(rndr, temp);

  return i;
}


static void
parse_block(Blob *out, const char *text, size_t size, Renderer *rndr)
{
  size_t start;
  char itemtype;
  int itemstart;
  rndr->nesting_depth++;
  for (start=0; start<size; ) {
    const char *ptr = text+start;
    size_t len, end = size-start;
    // * atx heading
    // * blockquote
    // * codeblock
    // * u/o list
    // * hrule
    // * fenced code
    // - table
    // - htmlblock   (see spec)
    // * paragraph
    if ((len = parse_atxheading(out, ptr, end, rndr))) {
      /* nothing else to do */
    }
    else if (is_codeline(ptr, end)) {
      len = parse_codeblock(out, ptr, end, rndr);
    }
    else if (is_quoteline(ptr, end)) {
      len = parse_blockquote(out, ptr, end, rndr);
    }
    else if ((len = is_hrule(ptr, end))) {
      do_hrule(out, rndr);
    }
    else if (is_fenceline(ptr, end)) {
      len = parse_fencedcode(out, ptr, end, rndr);
    }
    else if (is_itemline(ptr, end, &itemtype, &itemstart)) {
      len = parse_list(out, itemtype, itemstart, ptr, end, rndr);
    }
    else if ((len = is_linkdef(ptr, end))) {
      /* nothing to do, just skip it */
    }
    else if ((len = is_blankline(ptr, end))) {
      /* nothing to do, blank lines separate blocks */
    }
    else {
      /* Non-blank lines that cannot be interpreted otherwise
         form a paragraph in Markdown/CommonMark: */
      len = parse_paragraph(out, ptr, end, rndr);
    }

    if (len) start += len;
    else break;
  }
  rndr->nesting_depth--;
}


void
markdown(Blob *out, const char *text, size_t size, struct markdown *mkdn)
{
  size_t start;
  Renderer renderer;
  if (!text || !size || !mkdn) return;
  assert(out != NULL);

  // setup
  renderer.cb = *mkdn;
  renderer.udata = mkdn->udata;
  renderer.nesting_depth = 0;
  renderer.pool_index = 0;
  memset(renderer.blob_pool, 0, sizeof(renderer.blob_pool));

  // 1st pass: collect references
  for (start = 0; start < size; ) {
    size_t len = is_linkdef(text+start, size-start);
    if (len) start += len;
    else {
      // TODO pre process into buffer: expand tabs (size=4) and newlines to \n
      len = scanline(text+start, size-start);
      if (len) start += len;
      else break;
    }
  }

  // 2nd pass: do the rendering
  if (mkdn->prolog) mkdn->prolog(out, mkdn->udata);
  parse_block(out, text, size, &renderer);
  if (mkdn->epilog) mkdn->epilog(out, mkdn->udata);

  // cleanup
  assert(renderer.nesting_depth == 0);
  while (renderer.pool_index > 0) {
    blob_free(renderer.blob_pool[--renderer.pool_index]);
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

  mkdnhtml(&output, blob_str(&input), blob_len(&input), 0);

  fputs(blob_str(&output), stdout);
  fflush(stdout);

  blob_free(&input);
  blob_free(&output);

  return 0;
}

#endif
