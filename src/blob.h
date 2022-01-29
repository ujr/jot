#pragma once
#ifndef BLOB_H
#define BLOB_H

/* A growable char buffer, always \0-terminated */

#include <stdarg.h>
#include <stddef.h>

typedef struct blob Blob;

struct blob {
  char *buf;    /* buffer pointer, must be null-initialized */
  size_t len;   /* bytes used in buf (excluding terminating \0) */
  size_t size;  /* bytes allocated (even, including terminating \0) */
};              /* invariants: n+1 <= size and always terminated */

#define BLOB_INIT        {0,0,0}

#define blob_buf(bp)     ((void *) (bp)->buf)
#define blob_str(bp)     ((bp)->buf ? (bp)->buf : "")
#define blob_len(bp)     ((bp)->buf ? (bp)->len : 0)
#define blob_size(bp)    ((bp)->buf ? ((bp)->size & ~1) : 0)
#define blob_byte(bp, i) ((bp)->buf[i])
#define blob_failed(bp)  ((bp)->size & 1)

#define blob_add(bp,bq) blob_addbuf((bp), (bq)->buf, (bq)->len)
void blob_addchar(Blob *bp, int c);
void blob_addstr(Blob *bp, const char *z);
void blob_addbuf(Blob *bp, const char *buf, size_t len);
void blob_addfmt(Blob *bp, const char *fmt, ...);
void blob_addvfmt(Blob *bp, const char *fmt, va_list ap);

void *blob_prepare(Blob *bp, size_t dlen);
void blob_addlen(Blob *bp, size_t dlen);

void blob_trunc(Blob *bp, size_t len);
#define blob_clear(bp) blob_trunc((bp), 0);

int blob_compare(Blob *bp, Blob *bq);
void blob_trimend(Blob *bp);  /* remove trailing white space */
void blob_endline(Blob *bp);  /* add final newline, if missing */

void blob_free(Blob *bp);
void (*blob_nomem(void (*handler)(void)))(void);

int blob_check(int harder);  /* self checks, return true iff all ok */

#endif
