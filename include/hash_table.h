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
#define HITER GLUE3(hiter_, prefix, _t)

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
  int (*comp) (key_t, key_t);
} HTABLE;

typedef struct HITER {
  const HTABLE *h;
  uint64_t curr;
} HITER;

/* 
   update?

   set_comp
   get_size
   get_capacity
   remove
 
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
  h->comp = DEFAULT_COMP_TYPE(temp);

  return h;
}

void GLUE3(hash_, prefix, _set_hash)(HTABLE *h, uint64_t (*hash_func) (key_t)) {
  h->hash_func = hash_func;
}

void GLUE3(hash_, prefix, _set_comp)(HTABLE *h, int (*comp)(key_t, key_t)) {
  h->comp = comp;
}

int64_t GLUE3(hash_, prefix, _get_size) (const HTABLE *h) {
  return h->size;
}

int64_t GLUE3(hash_, prefix, _get_capacity) (const HTABLE *h) {
  return h->capacity;
}

void GLUE3(hash_, prefix, _destroy)(HTABLE **h_ptr) {
  HTABLE *h = *h_ptr;
  if (h == NULL) {
    return;
  }

  for (int64_t i = 0; i < h->capacity; i++) {
    free(h->A[i]);
  }
  free(h->A);

  h->capacity = 0;
  h->size = 0;
  h->A = NULL;
  h->hash_func = NULL;
  h->comp = NULL;

  free(h);
  h_ptr = NULL;
}

int32_t GLUE3(hash_, prefix, _rehash)(HTABLE *h) {
  if (h == NULL) {
    return -1;
  }
  int64_t new_capacity = 2 * h->capacity;
  HNODE **new_A = malloc(new_capacity * sizeof(HNODE*));
  if (new_A == NULL) {
    return -1;
  }
  for (int64_t i = 0; i < new_capacity; i++) {
    new_A[i] = NULL;
  }

  const uint64_t mask = new_capacity - UINT64_C(1);

  for (int64_t i = 0; i < h->capacity; i++) {
    HNODE *n = h->A[i];
    if (n == NULL) {
      continue;
    }
    uint64_t base = n->hash & mask;
    uint64_t step = ((n->hash / new_capacity) & mask) | UINT64_C(1);

    for (uint64_t pos = base; ; pos = (pos + step) & mask) {
      if (new_A[pos] == NULL) {
	new_A[pos] = n;
	break;
      }
    }
  }

  free(h->A);
  h->A = new_A;
  h->capacity = new_capacity;
  return 0;
}

int32_t GLUE3(hash_, prefix, _put)(HTABLE *h, key_t key, value_t value) {
  if (h == NULL || h->hash_func == NULL) {
    return -1;
  }
  const uint64_t hash = h->hash_func(key);
  const uint64_t mask = h->capacity - UINT64_C(1);
  const uint64_t base = hash & mask;
  const uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);
  
  for (uint64_t pos = base; ; pos = (pos + step) & mask) {
    if (h->A[pos] == NULL) {
      HNODE *n = malloc(sizeof(HNODE));
      if (n == NULL) {
	return -1;
      }
      n->hash = hash;
      n->key = key;
      n->value = value;
      h->A[pos] = n;
      h->size++;
      break;
    } else if (h->A[pos]->hash == hash) {
      h->A[pos]->value = value;
      break;
    }
  }

  if (h->size > LOAD_FACTOR * h->capacity) {
    int32_t rc = GLUE3(hash_, prefix, _rehash)(h);
    if (rc != 0) {
      return 0;
    }
  }
  return 0;
}

int32_t GLUE3(hash_, prefix, _get)(HTABLE *h, key_t key, value_t *value) {
  if (h == NULL || h->hash_func == NULL) {
    return -1;
  }
  uint64_t hash = h->hash_func(key);
  uint64_t mask = h->capacity - UINT64_C(1);
  uint64_t base = hash & mask;
  uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);
  
  for (uint64_t pos = base; ; pos = (pos + step) & mask) {
    if (h->A[pos] == NULL) {
      return 0;
    } else {
      if (h->A[pos]->hash == hash) {
	if (value != NULL) {
	  *value = h->A[pos]->value;
	}
	return 1;
      }
    }
  }
}

int32_t GLUE3(hash_, prefix, _remove)(HTABLE *h, key_t key, value_t *value) {
  if (h == NULL || h->hash_func == NULL) {
    return -1;
  }
  uint64_t hash = h->hash_func(key);
  uint64_t mask = h->capacity - UINT64_C(1);
  uint64_t base = hash & mask;
  uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);
  
  for (uint64_t pos = base; ; pos = (pos + step) & mask) {
    if (h->A[pos] == NULL) {
      return 0;
    } else {
      if (h->A[pos]->hash == hash) {
	if (value != NULL) {
	  *value = h->A[pos]->value;
	}
	free(h->A[pos]);
	h->A[pos] = NULL;
	h->size--;
	return 1;
      }
    }
  }
}

int32_t GLUE3(hash_, prefix, _next) (HITER **iter_ptr, key_t *key, value_t *value) {
  HITER *iter = *iter_ptr;
  const HTABLE *h = iter->h;
  uint64_t curr = iter->curr;

  while (1) {
    if (curr == h->capacity) {
      iter->h = NULL;
      iter->curr = -1;
      free(iter);
      *iter_ptr = NULL;
      return 1;
    }
    
    if (h->A[curr] != NULL) {
      if (key != NULL) {
	*key = h->A[curr]->key;
      }
      if (value != NULL) {
	*value = h->A[curr]->value;
      }
      curr++;
      iter->curr = curr;
      return 0;
    }
    
    curr++;
  }
}

int32_t GLUE3(hash_, prefix, _first) (const HTABLE *h, HITER **iter_ptr, key_t *key, value_t *value) {
  HITER *iter = malloc(sizeof(HITER));
  iter->h = h;
  iter->curr = 0;
  *iter_ptr = iter;

  return GLUE3(hash_, prefix, _next)(iter_ptr, key, value);
}
   
#undef HNODE
#undef HTABLE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
