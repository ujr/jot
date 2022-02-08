
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#include "memory.h"


/* default implementation delegates to std c lib */
static void *std_alloc(size_t n) { return malloc(n); }
static void *std_realloc(void *p, size_t n) { return realloc(p, n); }
static void  std_free(void *p) { free(p); }


static struct {
  void *(*alloc)(size_t);
  void *(*realloc)(void*, size_t);
  void  (*free)(void*);
} M = {
  std_alloc,
  std_realloc,
  std_free
};


void
mem_config(
  void *(*alloc)(size_t),
  void *(*realloc)(void *, size_t),
  void  (*free)(void *)
){
  assert(alloc);
  assert(realloc);
  assert(free);

  M.alloc = alloc;
  M.realloc = realloc;
  M.free = free;
}


void *
mem_alloc(size_t n)
{
  return M.alloc(n);
}

void *
mem_realloc(void *p, size_t n)
{
  return M.realloc(p, n);
}

void
mem_free(void *p)
{
  M.free(p);
}


#define DEFAULT_CHUNK_SIZE 4000  /* bytes */


void
mem_pool_init(MemPool *pool, size_t chunk_size)
{
  assert(pool);
  if (!chunk_size)
    chunk_size = DEFAULT_CHUNK_SIZE;
  memset(pool, 0, sizeof(*pool));
  pool->alloc = chunk_size;
}


void
mem_pool_free(MemPool *pool)
{
  MemChunk *chunk, *next;
  assert(pool);
  for (chunk = pool->chunk; chunk; chunk = next) {
    next = chunk->next;
    mem_free(chunk);
  }
  memset(pool, 0, sizeof(*pool));
}


void *
mem_pool_alloc(MemPool *pool, size_t n)
{
  void *p;
  assert(pool);
  n = (n+7)&~7;  /* round up to multiple of 8 */
  if (n > pool->alloc/2) {  /* large alloc gets its own chunk */
    MemChunk *chunk = mem_alloc(n+8);
    if (!chunk) return 0;
    chunk->next = pool->chunk;
    pool->chunk = chunk;
    return &((char*) chunk)[8];
  }
  if (pool->avail < n) {
    MemChunk *chunk = mem_alloc(pool->alloc + 8);
    if (!chunk) return 0;
    chunk->next = pool->chunk;
    pool->chunk = chunk;
    pool->ptr = (char *) chunk;
    pool->ptr += 8;
    pool->avail = pool->alloc;
  }
  p = pool->ptr;
  pool->ptr += n;
  pool->avail -= n;
  return p;
}
