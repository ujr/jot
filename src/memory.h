
#include <stddef.h>

void mem_config(void *(*ma)(size_t), void *(*ra)(void*,size_t), void (*mf)(void*));

void *mem_alloc(size_t n);
void *mem_realloc(void *p, size_t n);
void  mem_free(void *p);

/* Pooled Memory Allocation: allows allocation from
   larger chunks that can be released all at once;
   got the idea from Hipp's unql code */

typedef struct mempool MemPool;
typedef struct memchunk MemChunk;

struct memchunk {
  MemChunk *next;       /* link to next mem chunk */
};

struct mempool {
  MemChunk *chunk;      /* head of list of mem chunks */
  char *ptr;            /* memory for allocation in this chunk */
  size_t avail;         /* bytes still available in this chunk */
  size_t alloc;         /* chunk size */
};

#define POOL_INIT { 0, 0, 0, 0 }

void mem_pool_init(MemPool *pool, size_t chunk_size);
void *mem_pool_alloc(MemPool *pool, size_t n);
char *mem_pool_dup(MemPool *pool, const char *s, size_t n);
void mem_pool_free(MemPool *pool);
