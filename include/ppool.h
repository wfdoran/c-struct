#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

/* A thread safe pool: the pool of pool.h with one mutex around get and put.
   See doc/ppool.md.  The pool inside is an instantiation of pool.h with a label
   of its own, so a pool.h and a ppool.h of the same prefix can both be used.
   The pool header defines and undefines the same helper macros as this one, so
   it is included first and the macros are defined after it. */
#pragma push_macro("pool_label")
#undef pool_label
#define pool_label GLUE(pp_, prefix)
#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)
#include "pool.h"
#undef pool_label
#pragma pop_macro("pool_label")
#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define TYPE GLUE3(ppool_, prefix, _t)
#define INNER GLUE3(pool_pp_, prefix, _t)

typedef struct TYPE {
  INNER *pool;
  pthread_mutex_t lock;
} TYPE;

/* ppool_prefix_t* ppool_prefix_init(size_t pool_size);

   Allocates a pool with room for pool_size objects of type data_t, as
   pool_prefix_init() does.

   Returns NULL if pool_size is 0, if the block would be too large to
   allocate, or if memory could not be allocated.
*/
static inline TYPE *GLUE3(ppool_, prefix, _init)(size_t pool_size) {
  TYPE *p = malloc(sizeof(TYPE));
  if (p == NULL) {
    return NULL;
  }
  p->pool = GLUE3(pool_pp_, prefix, _init)(pool_size);
  if (p->pool == NULL) {
    free(p);
    return NULL;
  }
  if (pthread_mutex_init(&(p->lock), NULL) != 0) {
    GLUE3(pool_pp_, prefix, _destroy)(&p->pool);
    free(p);
    return NULL;
  }
  return p;
}

/* data_t* ppool_prefix_get(ppool_prefix_t *pool);

   Returns space for one data_t, as pool_prefix_get() does, and may be called
   from any number of threads at once.  The space is not cleared.  Space which
   was given back is handed out again first, the most recently given back
   first.

   Returns NULL if pool is NULL, or if the pool is full and memory for another
   block could not be allocated.
*/
static inline data_t *GLUE3(ppool_, prefix, _get)(TYPE *pool) {
  if (pool == NULL) {
    return NULL;
  }
  pthread_mutex_lock(&(pool->lock));
  data_t *d = GLUE3(pool_pp_, prefix, _get)(pool->pool);
  pthread_mutex_unlock(&(pool->lock));
  return d;
}

/* int32_t ppool_prefix_put(ppool_prefix_t *pool, data_t **d_ptr);

   Gives back space which ppool_prefix_get() returned, as pool_prefix_put()
   does, and may be called from any number of threads at once.  On success
   *d_ptr is set to NULL.  Space may be given back by a different thread than
   the one which got it.

   Space which is not in use in this pool (given back already, or from another
   pool) is refused.  If two threads give back the same space, one of the calls
   succeeds and the other returns -1; that is a bug of the caller all the same,
   as using the space after giving it back is.

   return value:
     -1 => error (pool, d_ptr or *d_ptr is NULL, or the space is not in use in
           this pool); nothing is changed, and *d_ptr is left as it is
      0 => ok
*/
static inline int32_t GLUE3(ppool_, prefix, _put)(TYPE *pool, data_t **d_ptr) {
  if (pool == NULL || d_ptr == NULL) {
    return -1;
  }
  pthread_mutex_lock(&(pool->lock));
  int32_t rc = GLUE3(pool_pp_, prefix, _put)(pool->pool, d_ptr);
  pthread_mutex_unlock(&(pool->lock));
  return rc;
}

/* int32_t ppool_prefix_destroy(ppool_prefix_t **p_ptr);

   Frees the pool with all of its blocks, and sets *p_ptr to NULL.  Every
   pointer from ppool_prefix_get() is invalid afterwards.

   No other thread may be using the pool, or be about to: destroy does not wait
   for calls in progress.  Join or otherwise stop the threads first.

   return value:
     -1 => nothing to destroy (p_ptr or *p_ptr is NULL)
      0 => ok
*/
static inline int32_t GLUE3(ppool_, prefix, _destroy)(TYPE **p_ptr) {
  if (p_ptr == NULL) {
    return -1;
  }
  TYPE *p = *p_ptr;
  if (p == NULL) {
    return -1;
  }
  GLUE3(pool_pp_, prefix, _destroy)(&p->pool);
  pthread_mutex_destroy(&(p->lock));
  free(p);
  *p_ptr = NULL;
  return 0;
}

#undef TYPE
#undef INNER
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
