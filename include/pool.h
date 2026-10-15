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

typedef struct ENTRY {
  data_t data;
  struct ENTRY *next;
} ENTRY;

typedef struct TYPE {
  struct ENTRY *alloc;
  struct ENTRY *head;
  struct TYPE *next;
  size_t pool_size;
} TYPE;


static inline TYPE *GLUE3(pool_, prefix, _init)(size_t pool_size) {
  if (pool_size == 0) {
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
  for (int32_t i = 0; i < pool_size - 1; i++) {
    pool->alloc[i].next = &(pool->alloc[i+1]);
  }
  pool->alloc[pool_size - 1].next = NULL;

  pool->next = NULL;
  pool->pool_size = pool_size;
  return pool;
}

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
  rv->next = NULL;

  return (data_t*) rv;
}

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
  ENTRY *e = (ENTRY*) d;
  if (e->next != NULL) {
    return -1;
  }
  e->next = pool->head;
  pool->head = e;

  *d_ptr = NULL;
  return 0;
}

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
