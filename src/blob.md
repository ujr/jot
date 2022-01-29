# The blob.h API

Provides growable byte buffers that are always
zero-terminated and therefore valid C strings.

```C
#include "blob.h"

Blob blob = BLOB_INIT;   /* mandatory initialization */
Blob *bp = &blob;

void *p;
char *s;
size_t n;
int i;

p = blob_buf(bp);        /* pointer to start of buffer */
s = blob_str(bp);        /* char pointer to start of buffer */
n = blob_len(bp);        /* blob length in bytes */
n = blob_size(bp);       /* allocated size in bytes */
c = blob_byte(bp, i);    /* blob byte at offset i */
blob_byte(bp, i) = c;
if (blob_failed(bp)) error();

blob_add(bp, Blob *bq);  /* append another blob */
blob_addchar(bp, int c); /* append a single byte */
blob_addstr(bp, const char *z);
blob_addbuf(bp, const char *buf, size_t len);
blob_addfmt(bp, const char *fmt, ...);
blob_addvfmt(bp, const char *fmt, va_list ap);

p = blob_prepare(bp, size_t dn);
blob_addlen(bp, size_t dn);
blob_trunc(bp, size_t n);

i = blob_compare(bp, Blob *bq);  /* like strcmp(3) */

blob_free(bp);          /* free memory, go to unallocated state */
blob_nomem(handler);    /* register out-of-memory handler */
blob_check(harder);     /* self checks, return true iff all ok */
```

A blob is in one of three states: unallocated
(buf==0, the initial state), normal (buf!=0), or
failed (after a memory allocation failed).

A blob MUST be initialized as in `Blob blob = BLOB_INIT;`
to start in the unallocated state. Without this initialization
all operations are undefined.

**blob_buf** returns an untyped pointer to the blob's data
area. If your blob contains text, prefer **blob_str** instead.

**blob_str** returns a character pointer to the blob's data
area. Since a blob guarantees that its contents is always
zero-terminated, this pointer can be used with functions
like **puts**(3) or **strcmp**(3).

**blob_len** returns the length of the blob in bytes, not
counting the terminating zero. **blob_size** returns the
blob's capacity (allocated size) in bytes; it will be
automatically reallocated if the blob grows.

**blob_byte** is the byte (`char`) at position *i* for
reading or writing; the blob must be allocated and *i*
in range, which is not checked.

**blob_failed** returns true after a memory allocation
has failed. By default, the program is aborted if a
memory allocation fails and the string buffer thus
never enters into failed state. This can be changed
by registering an error handler with **blob_nomem**.
The handler could report an error and/or **longjmp**(3).
If the handler returns, the program will still be aborted.
**blob_nomem** returns the previous error handler.

The **blob_add** functions append to the end of the blob:
**add** the contents of another blob,
**addchar** a character (`int` cast to `unsigned char`),
**addstr** a zero-terminated string,
**addbuf** a buffer of the given length,
**addfmt** and **addvfmt** a formatted string (see **sprintf**(3)).
They all grow the blob if necessary. If a memory allocation
fails, the blob goes to failed state, the handler is invoked,
and if it returns, the program is aborted.

**blob_prepare** ensures the blob has enough capacity to hold
the given number of additional bytes; it returns a pointer
to the new area within the blob, so it is readily available.
After modifying this new area with non-library functions,
call **blob_addlen** to add those new bytes to the blob.

**blob_trunc** truncates the blob to the given length, which
must be no more than the current length. The library guarantees
that the buffer is again zero-terminated after truncation.
**blob_clear** is equivalent to truncating to length zero.

**blob_free** empties the blob, releases the memory, and
returns the blob to the unallocated state (which also clears
the failed flag).
