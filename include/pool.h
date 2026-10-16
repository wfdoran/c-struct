#include <stdint.h>
#include <stdlib.h>

#include "cs_private.h"

#ifndef data_t
#error "data_t not defined"
#endif

/* pool_label is the name part of the pool types and functions, normally the
   prefix.  The hash tables, which keep a pool of their nodes, set it to a label
   of their own (and data_t to the node type), so that the pool inside a table
   cannot clash with a pool of the same prefix made by the user.  Users do not
   set it. */
#ifndef pool_label
#ifndef prefix
#error "prefix not defined"
#endif
#define pool_label prefix
#define POOL_LABEL_IS_PREFIX
#endif

/* Each new block is CS_POOL_GROWTH_NUM / CS_POOL_GROWTH_DEN times the size of
   the last one, 3 / 2 = 1.5 unless it is set with -D (the numerator must be
   larger than the denominator).  It is kept as a fraction so that the size is
   computed exactly, in integers.  A smaller factor wastes less memory in the
   last block, a larger one allocates less often. */
#ifndef CS_POOL_GROWTH_NUM
#define CS_POOL_GROWTH_NUM 3
#endif
#ifndef CS_POOL_GROWTH_DEN
#define CS_POOL_GROWTH_DEN 2
#endif

/* CS_POOL_NEXT_SIZE_DEFINED is internal: it makes the function below be defined
   once however many pools are instantiated. */
#ifndef CS_POOL_NEXT_SIZE_DEFINED
#define CS_POOL_NEXT_SIZE_DEFINED
_Static_assert(CS_POOL_GROWTH_NUM > CS_POOL_GROWTH_DEN,
               "a pool must grow: CS_POOL_GROWTH_NUM > CS_POOL_GROWTH_DEN");

/* The size of the block which follows a block of n objects: n times the growth
   factor, rounded up, so it is always larger than n.  Saturates at SIZE_MAX,
   which pool_prefix_init() then refuses as too large. */
CS_PRIVATE static inline size_t cs_pool_next_size(size_t n) {
  const size_t extra = CS_POOL_GROWTH_NUM - CS_POOL_GROWTH_DEN;
  /* n * extra / DEN, rounded up, without overflowing in n * extra */
  const size_t grow =
      n / CS_POOL_GROWTH_DEN * extra +
      (n % CS_POOL_GROWTH_DEN * extra + CS_POOL_GROWTH_DEN - 1) /
          CS_POOL_GROWTH_DEN;
  return grow > SIZE_MAX - n ? SIZE_MAX : n + grow;
}
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define TYPE GLUE3(pool_, pool_label, _t)
#define ENTRY GLUE3(pentry_, pool_label, _t)

/* While an entry is free, next links it to the next free entry.  While it is in
   use, next holds the pool, which is how pool_prefix_put() recognizes space
   that was not handed out by this pool or was already given back. */
typedef struct ENTRY {
  data_t data;
  struct ENTRY *next;
} ENTRY;

/* The pool is a chain of blocks, each one allocation of pool_size entries.  The
   pool the user holds is the first block.  Only its head is used: it is the
   list of the free entries of all the blocks.  A block is never moved or freed
   before the pool is destroyed, so a pointer from pool_prefix_get() stays valid
   while the pool grows. */
typedef struct TYPE {
  struct ENTRY *alloc;
  struct ENTRY *head;
  struct TYPE *next;
  size_t pool_size;
} TYPE;

/* pool_prefix_t* pool_prefix_init(size_t pool_size);

   Allocates a pool with room for pool_size objects of type data_t.  When all of
   them are in use, pool_prefix_get() adds a block 1.5 times as large as the
   last one (rounded up; CS_POOL_GROWTH_NUM / CS_POOL_GROWTH_DEN), so the pool
   never runs out as long as memory lasts.

   All the entries of the block are linked into the list of free entries now, so
   this takes time proportional to pool_size.

   Returns NULL if pool_size is 0, if the block would be too large to
   allocate, or if memory could not be allocated.
*/
static inline TYPE *GLUE3(pool_, pool_label, _init)(size_t pool_size) {
  if (pool_size == 0 || pool_size > SIZE_MAX / sizeof(ENTRY)) {
    return NULL;
  }
  TYPE *pool = malloc(sizeof(TYPE));
  if (pool == NULL) {
    return NULL;
  }
  pool->alloc = malloc(pool_size * sizeof(ENTRY));
  if (pool->alloc == NULL) {
    return NULL;
  }

  pool->head = pool->alloc;
  for (size_t i = 0; i + 1 < pool_size; i++) {
    pool->alloc[i].next = &(pool->alloc[i + 1]);
  }
  pool->alloc[pool_size - 1].next = NULL;

  pool->next = NULL;
  pool->pool_size = pool_size;
  return pool;
}

/* data_t* pool_prefix_get(pool_prefix_t *pool);

   Returns space for one data_t, or NULL if pool is NULL or the pool is full and
   memory for a new block could not be allocated (the pool is unchanged then).

   The space is not cleared: it holds whatever was there, like the result of
   malloc().  It belongs to the caller until it is given back with
   pool_prefix_put(), or the pool is destroyed.  Space which was given back is
   handed out again first, the most recently given back first.

   Not thread safe: calls on one pool must not overlap (ppool.h has a mutex).
*/
static inline data_t *GLUE3(pool_, pool_label, _get)(TYPE *pool) {
  if (pool == NULL) {
    return NULL;
  }

  if (pool->head == NULL) {
    TYPE *p = pool;
    while (p->next != NULL) {
      p = p->next;
    }
    TYPE *p2 = GLUE3(pool_, pool_label, _init)(cs_pool_next_size(p->pool_size));
    if (p2 == NULL) {
      return NULL;
    }
    p->next = p2;
    pool->head = p2->head;
    p2->head = NULL;
  }

  ENTRY *rv = pool->head;
  pool->head = rv->next;
  rv->next = (ENTRY *) pool; /* in use: marked with the pool it came from */

  return (data_t *) rv;
}

/* int32_t pool_prefix_put(pool_prefix_t *pool, data_t **d_ptr);

   Gives back space which pool_prefix_get() returned, so that it can be handed
   out again.  On success *d_ptr is set to NULL, so that the same variable
   cannot be given back twice.  The space must not be used after it is given
   back.

   The space must come from this pool.  A second put of space which is not in
   use (through a copy of the pointer), and space which belongs to another pool,
   are refused.  A pointer which does not come from any pool cannot be
   recognized: put reads the memory next to the data to find out.

   return value:
     -1 => error (pool, d_ptr or *d_ptr is NULL, or the space is not in use in
           this pool); nothing is changed, and *d_ptr is left as it is
      0 => ok

   Not thread safe: calls on one pool must not overlap (ppool.h has a mutex).
*/
static inline int32_t GLUE3(pool_, pool_label, _put)(TYPE *pool,
                                                     data_t **d_ptr) {
  if (pool == NULL) {
    return -1;
  }
  if (d_ptr == NULL) {
    return -1;
  }

  data_t *d = *d_ptr;
  if (d == NULL) {
    return -1;
  }
  ENTRY *e = (ENTRY *) d;
  if (e->next != (ENTRY *) pool) {
    return -1; /* not in use in this pool: given back already, or not ours */
  }
  e->next = pool->head;
  pool->head = e;

  *d_ptr = NULL;
  return 0;
}

/* int32_t pool_prefix_destroy(pool_prefix_t **p_ptr);

   Frees the pool and all of its blocks, and sets *p_ptr to NULL.  Every pointer
   from pool_prefix_get() is invalid afterwards, whether or not it was given
   back; the objects are not destroyed one by one, so if a data_t owns memory
   that has to be freed, do that first.

   return value:
     -1 => nothing to destroy (p_ptr or *p_ptr is NULL)
      0 => ok
*/
static inline int32_t GLUE3(pool_, pool_label, _destroy)(TYPE **p_ptr) {
  if (p_ptr == NULL) {
    return -1;
  }
  TYPE *pool = *p_ptr;
  if (pool == NULL) {
    return -1;
  }

  if (pool->next != NULL) {
    int32_t rc = GLUE3(pool_, pool_label, _destroy)(&pool->next);
    if (rc != 0) {
      return rc;
    }
  }
  free(pool->alloc);
  pool->alloc = NULL;
  pool->head = NULL;
  free(pool);
  *p_ptr = NULL;
  return 0;
}

#undef TYPE
#undef ENTRY
#ifdef POOL_LABEL_IS_PREFIX
#undef pool_label
#undef POOL_LABEL_IS_PREFIX
#endif
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
