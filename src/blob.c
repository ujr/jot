
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>  /* size_t */
#include <stdio.h>   /* vsnprintf() */
#include <stdlib.h>
#include <string.h>


#include "blob.h"


#define GROWFUNC(x) (((x)+16)*3/2)  /* lifted from Git */
#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define ISSPACE(x)  isspace(x)

#define LEN(sp)     ((sp)->len)
#define SIZE(sp)    ((sp)->size & ~1)           /* mask off lsb */
#define HASROOM(sp,n) (LEN(sp)+(n)+1 <= SIZE(sp))  /* +1 for \0 */
#define NEXTSIZE(sp)  GROWFUNC(SIZE(sp))   /* next default size */
#define SETFAILED(sp) ((sp)->size |= 1)      /* set lsb to flag */


static void (*nomem)(void) = 0;

/* register new handler, return old handler */
void (*blob_nomem(void (*handler)(void)))(void)
{
  void *old = nomem;
  nomem = handler;
  return old;
}


void  /* append the single char/byte c */
blob_addchar(Blob *bp, int c)
{
  assert(bp != 0);
  if (!bp->buf || !HASROOM(bp, 1)) {
    if (!blob_prepare(bp, 1)) return; /* nomem */
  }
  bp->buf[bp->len++] = (unsigned char) c;
  bp->buf[bp->len] = '\0';
}


void  /* append the \0 terminated string z */
blob_addstr(Blob *bp, const char *z)
{
  blob_addbuf(bp, z, z ? strlen(z) : 0);
}


void  /* append buf[0..len-1] */
blob_addbuf(Blob *bp, const char *buf, size_t len)
{
  assert(bp != 0);
  if (!buf) len = 0;
  if (!blob_prepare(bp, len)) return; /* nomem */
  if (buf) memcpy(bp->buf + bp->len, buf, len);
  bp->len += len;
  bp->buf[bp->len] = '\0';
}


void  /* append formatted string (variadic) */
blob_addfmt(Blob *bp, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  blob_addvfmt(bp, fmt, ap);
  va_end(ap);
}


void  /* append formatted string (va_list) */
blob_addvfmt(Blob *bp, const char *fmt, va_list ap)
{
  va_list aq;
  int chars;

  /* Make a copy of ap because we may traverse the list twice (if we
     need to grow the buffer); also notice that va_copy() requires a
     matching va_end(), and that the size argument to vsnprintf()
     includes the terminating \0, whereas its return value does not. */

  va_copy(aq, ap); /* C99 */
  chars = vsnprintf(bp->buf + bp->len, 0, fmt, aq);
  va_end(aq);

  if (chars < 0) return;
  if (!blob_prepare(bp, chars)) return; /* nomem */

  chars = vsnprintf(bp->buf + bp->len, chars+1, fmt, ap);
  if (chars < 0) return;

  bp->len += chars;
}


char * /* ensure enough space for plus more bytes */
blob_prepare(Blob *bp, size_t plus)
{
  assert(bp != 0);
  /* nothing to do if allocated and enough room: */
  if (bp->buf && HASROOM(bp, plus))
    return bp->buf + bp->len;

  size_t requested = bp->len + plus + 1; /* +1 for \0 */
  size_t standard = GROWFUNC(SIZE(bp));
  size_t newsize = MAX(requested, standard);

  newsize = (newsize+1)&~1; /* round up to even */
  char *ptr = realloc(bp->buf, newsize);
  if (!ptr) goto nomem;
  memset(ptr + bp->len, 0, newsize - bp->len);

  bp->buf = ptr;
  bp->size = newsize;
  return bp->buf + bp->len; /* ptr to new part of blob */

nomem:
  SETFAILED(bp);
  if (nomem) nomem();
  else perror("blob");
  abort();
  return 0;
}


void  /* make plus bytes longer, after prepare(plus) */
blob_addlen(Blob *bp, size_t plus)
{
  assert(bp && bp->buf);
  size_t newlen = bp->len + plus;
  size_t max = bp->size - 1;
  if (newlen > max) newlen = max;
  bp->len = newlen;
  bp->buf[bp->len] = '\0';
}


void  /* truncate string to exactly n <= len chars */
blob_trunc(Blob *bp, size_t n)
{
  assert(bp != 0);
  if (!bp->buf) return;  /* not allocated */
  if (n > bp->len) return; /* cannot enlarge */
  bp->len = n;
  bp->buf[n] = '\0';
}

int  /* return neg/0/pos if bp is </=/> bq */
blob_compare(Blob *bp, Blob *bq)
{
  size_t np = blob_len(bp);
  size_t nq = blob_len(bq);
  size_t n = np < nq ? np : nq;
  int r = memcmp(blob_str(bp), blob_str(bq), n);
  if (r != 0) return r;
  if (np == nq) return 0;
  return np > nq ? 1 : -1;
}


void  /* trim trailing white space */
blob_trimend(Blob *bp)
{
  assert(bp != 0);
  if (!bp->buf) return;
  char *z = bp->buf;
  size_t n = bp->len;
  while (n > 0 && ISSPACE(z[n-1])) n--;
  bp->len = n;
  bp->buf[bp->len] = '\0';
}


void  /* add final newline, if missing */
blob_endline(Blob *bp)
{
  assert(bp != 0);
  if (!bp->buf) return;  /* not allocated */
  if (!bp->len) return;  /* empty, no line to end */
  if (bp->buf[bp->len-1] != '\n')
    blob_addchar(bp, '\n');
}


void  /* release memory, set to unallocated */
blob_free(Blob *bp)
{
  assert(bp != 0);
  if (bp->buf) {
    free(bp->buf);
    bp->buf = 0;
  }
  bp->len = bp->size = 0;
}


/** remainder of file is self check code **/

#include "log.h"

#ifdef __unix__
#include <unistd.h>  /* provides _POSIX_VERSION */
#endif

#ifdef _POSIX_VERSION
#include <setjmp.h>
#include <sys/resource.h>  /* setrlimit */
static jmp_buf jbuf;
static void out_of_memory(void) { longjmp(jbuf, 1); }
#endif

#define INFO(...) log_debug(__VA_ARGS__)
#define TEST(name, cond) do{ \
    if (cond) {log_trace("PASS: %s", (name)); numpass++;} \
    else {log_debug("FAIL: %s", (name)); numfail++;} \
  }while(0)

#define STREQ(s,t) strcmp((s),(t))==0

#define LENLTSIZE(bp)  (blob_len(bp) < blob_size(bp))
#define TERMINATED(bp) (0 == (bp)->buf[blob_len(bp)])
#define INVARIANTS(bp) (LENLTSIZE(bp) && TERMINATED(bp))

#define blob_setstr(bp, s) blob_clear((bp)); blob_addstr((bp), (s))

int  /* run self checks; return true iff all ok */
blob_check(int harder)
{
  /* volatile because of setjmp/longjmp */
  volatile int numpass = 0;
  volatile int numfail = 0;
  int i;

  Blob blob = BLOB_INIT;
  Blob *bp = &blob;
  Blob other = BLOB_INIT;

  INFO("sizeof(Blob): %zu bytes", sizeof(Blob));

  TEST("init len = 0", blob_len(bp) == 0);
  TEST("init eq \"\"", STREQ(blob_str(bp), ""));

  blob_addbuf(bp, "Hellooo", 5);
  TEST("addbuf", STREQ(blob_str(bp), "Hello") && INVARIANTS(bp));

  blob_addchar(bp, ',');
  blob_addchar(bp, ' ');
  TEST("addchar", STREQ(blob_str(bp), "Hello, ") && INVARIANTS(bp));

  blob_addstr(bp, "World!");
  TEST("addstr", STREQ(blob_str(bp), "Hello, World!") && INVARIANTS(bp));

  blob_trunc(bp, 5);
  TEST("trunc 5", STREQ(blob_str(bp), "Hello") && INVARIANTS(bp));

  blob_addfmt(bp, "+%d-%d=%s", 3, 4, "konfus");
  TEST("addfmt", STREQ(blob_str(bp), "Hello+3-4=konfus") && INVARIANTS(bp));

  blob_byte(bp, 10) = 'Q';
  TEST("byte", blob_byte(bp, 10) == 'Q');

  blob_trunc(bp, 0);
  TEST("trunc 0 (buf)", STREQ(blob_str(bp), "") && INVARIANTS(bp));
  TEST("trunc 0 (len)", blob_len(bp) == 0 && INVARIANTS(bp));

  for (i = 0; i < 26; i++) {
    blob_addchar(bp, "abcdefghijklmnopqrstuvwxyz"[i]);
    blob_addchar(bp, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]);
  }
  TEST("52*addchar", blob_len(bp) == 52 && INVARIANTS(bp) &&
    STREQ(blob_str(bp), "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ"));

  blob_trunc(bp, 0);
  TEST("trunc 0", blob_len(bp) == 0 && INVARIANTS(bp));

  blob_addbuf(bp, "\0\a\b\f\n\r\t\v\0", 9);
  TEST("embedded \\0", blob_len(bp) == 9 && INVARIANTS(bp));

  blob_free(bp);
  TEST("free", bp->buf == 0 && blob_len(bp) == 0 && blob_size(bp) == 0);

  blob_addstr(bp, "From Moby Dick:\n"); /* 1st alloc after free() */
  blob_addstr(bp, /* will trigger an exact alloc */
"Call me Ishmael. Some years ago—never mind how long precisely—having "
"little or no money in my purse, and nothing particular to interest me "
"on shore, I thought I would sail about a little and see the watery part "
"of the world. It is a way I have of driving off the spleen and "
"regulating the circulation. Whenever I find myself growing grim about "
"the mouth; whenever it is a damp, drizzly November in my soul; whenever "
"I find myself involuntarily pausing before coffin warehouses, and "
"bringing up the rear of every funeral I meet; and especially whenever "
"my hypos get such an upper hand of me, that it requires a strong moral "
"principle to prevent me from deliberately stepping into the street, and "
"methodically knocking people’s hats off—then, I account it high time to "
"get to sea as soon as I can. This is my substitute for pistol and ball. "
"With a philosophical flourish Cato throws himself upon his sword; I "
"quietly take to the ship. There is nothing surprising in this. If they "
"but knew it, almost all men in their degree, some time or other, "
"cherish very nearly the same feelings towards the ocean with me.");
  TEST("large addstr (len+1==size)", blob_len(bp)+1 == blob_size(bp) && INVARIANTS(bp));
  INFO("size=%zu, len=%zu", blob_size(bp), blob_len(bp));

  blob_addchar(bp, '\n');
  TEST("single addchar (len+1<size)", blob_len(bp)+1 < blob_size(bp) && INVARIANTS(bp));
  INFO("size=%zu, len=%zu", blob_size(bp), blob_len(bp));
  TEST("not failed", !blob_failed(bp));

  blob_free(bp);

  blob_addstr(bp, ""); /* length 0, must still trigger alloc for \0 */
  blob_addbuf(bp, "Hellooo", 5);
  blob_addchar(bp, ' ');
  blob_addstr(bp, "World!");
  blob_trunc(bp, 6);
  blob_addfmt(bp, "User #%d", 123);
  INFO("%s (len=%zu)", blob_str(bp), blob_len(bp));
  TEST("sample", STREQ(blob_str(bp), "Hello User #123") && INVARIANTS(bp));

  blob_free(bp);

  blob_setstr(bp, "baz");
  blob_setstr(&other, "baz");
  TEST("compare1", blob_compare(bp, bp) == 0);
  TEST("compare2", blob_compare(bp, &other) == 0);
  blob_setstr(&other, "bar");
  TEST("compare3", blob_compare(bp, &other) > 0);
  TEST("compare4", blob_compare(&other, bp) < 0);
  blob_setstr(&other, "bazaar");
  TEST("compare5", blob_compare(bp, &other) < 0);
  TEST("compare6", blob_compare(&other, bp) > 0);
  blob_clear(bp);
  blob_clear(&other);
  TEST("compare0", blob_compare(bp, &other) == 0);

  blob_free(bp);

  /* Exercise allocation through addchar(): */
  for (i = 0; i < 100*1024*1024; i++) {
    blob_addchar(bp, "abcdefghijklmnopqrstuvwxyz"[i%26]);
  }
  TEST("100M*addchar", blob_len(bp) == 100*1024*1024 && INVARIANTS(bp));
  INFO("size=%zu, len=%zu", blob_size(bp), blob_len(bp));
  blob_free(bp);

  /* Exercise allocation through addbuf(): */
  for (i = 0; i < 5*1024*1024; i++) {
    blob_addbuf(bp, "01234567890123456789", 20);
  }
  TEST("5M*addbuf", blob_len(bp) == 100*1024*1024 && INVARIANTS(bp));
  INFO("size=%zu, len=%zu", blob_size(bp), blob_len(bp));
  blob_free(bp);

  /* Exercise allocation through addfmt(): */
  for (i = 0; i < 5*1024*1024; i++) {
    blob_addfmt(bp, "appending fmt: %zu", (size_t) i);
  }
  TEST("5M*addfmt", INVARIANTS(bp));
  INFO("size=%zu, len=%zu", blob_size(bp), blob_len(bp));
  blob_free(bp);

#ifdef _POSIX_VERSION
  if (harder) {
    void (*oldhandler)(void);
    struct rlimit maxmem;
    INFO("_POSIX_VERSION = %ld", _POSIX_VERSION);
    /* Limit memory so this is reasonably fast */
    maxmem.rlim_cur = 4*1024*1024;
    maxmem.rlim_max = 4*1024*1024;
    setrlimit(RLIMIT_DATA, &maxmem);
    /* Register error handler to replace the default abort() */
    oldhandler = blob_nomem(out_of_memory);
    if (setjmp(jbuf)==0) {
      for (;;) {
        blob_addstr(bp, "Appending zero-terminated string until memory failure\n");
      }
   }
    TEST("alloc until failure", blob_failed(bp) && INVARIANTS(bp));
    INFO("size=%zu, len=%zu, failed=%d", blob_size(bp), blob_len(bp), blob_failed(bp));
    blob_free(bp);
    blob_nomem(oldhandler);
  }
#endif

  blob_free(bp);
  blob_free(&other);

  return numpass > 0 && numfail == 0;
}
