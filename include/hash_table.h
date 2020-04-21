#include <stdint.h>
#include <stdlib.h>
#include "hash.h"
#include "comp.h"

#ifndef key_t
#error "key_t not defined"
#endif

#ifndef value_t
#error "value_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define HTABLE GLUE3(htable_, prefix, _t)
#define HNODE GLUE3(hnode_, prefix, _t)

#define LOAD_FACTOR (0.75)

typedef struct HNODE {
  uint64_t hash;
  key_t key;
  value_t value;
} HNODE;

typedef struct HLIST {
  int64_t capacity;
  int64_t size;
  HNODE **A;
  uint64_t (*hash_func) (key_t);
  int (*comp) (key_t *, key_t *);
} HTABLE;

/* 
   HLIST *GLUE3(tree_, prefix, _init) (size_t expected_size);
   void GLUE3(tree_, prefix, _destroy) (HTABLE

   init
   destroy
   set_hash
   get_size
   get_capacity
   put
   get
   remove

   rehash

   first
   next
 
https://en.wikipedia.org/wiki/Hash_table
*/

static int64_t GLUE3(hash_, prefix, _roundup_pow2) (int64_t x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  x++;
  return x;
}

HTABLE *GLUE3(hash_, prefix, _init) (int64_t expected_size) {
  HTABLE* h = malloc(sizeof(HTABLE));
  if (h == NULL) {
    return NULL;
  }
  
  h->capacity = expected_size <= 16 ? 16 : GLUE3(hash_, prefix, _roundup_pow2)(expected_size);
  h->size = 0;
  h->A = malloc(h->capacity * sizeof(HNODE*));
  if (h->A == NULL) {
    return NULL;
  }
  for (int64_t i = 0; i < h->capacity; i++) {
    h->A[i] = NULL;
  }
  key_t temp;
  h->hash_func = DEFAULT_HASH(temp);
  h->comp = DEFAULT_COMP_PTR(temp);
}


#undef HNODE
#undef HTABLE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
