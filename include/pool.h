#include <stdint.h>
#include <stdlib.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define TYPE GLUE3(pool_, prefix, _t)
#define ENTRY GLUE3(pentry_, prefix, _t)

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
   them are in use, pool_prefix_get() adds a block twice as large as the last
   one, so the pool never runs out as long as memory lasts.

   All the entries of the block are linked into the list of free entries now, so
   this takes time proportional to pool_size.

   Returns NULL if pool_size is 0, if the block would be too large to
   allocate, or if memory could not be allocated.
*/
static inline TYPE *GLUE3(pool_, prefix, _init)(size_t pool_size) {
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

   Not thread safe: calls on one pool must not overlap.
*/
static inline data_t *GLUE3(pool_, prefix, _get)(TYPE *pool) {
  if (pool == NULL) {
    return NULL;
  }

  if (pool->head == NULL) {
    TYPE *p = pool;
    while (p->next != NULL) {
      p = p->next;
    }
    TYPE *p2 = GLUE3(pool_, prefix, _init)(p->pool_size * 2);
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

   Not thread safe: calls on one pool must not overlap.
*/
static inline int32_t GLUE3(pool_, prefix, _put)(TYPE *pool, data_t **d_ptr) {
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
static inline int32_t GLUE3(pool_, prefix, _destroy)(TYPE **p_ptr) {
  if (p_ptr == NULL) {
    return -1;
  }
  TYPE *pool = *p_ptr;
  if (pool == NULL) {
    return -1;
  }

  if (pool->next != NULL) {
    int32_t rc = GLUE3(pool_, prefix, _destroy)(&pool->next);
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
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
