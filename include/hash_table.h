#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "comp.h"

#ifndef hkey_t
#error "hkey_t not defined"
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

/* Basic structs used in the header file. 
 
*/
typedef struct HNODE {
    uint64_t hash;
    hkey_t key;
    value_t value;
} HNODE;

typedef struct HTABLE {
    int64_t capacity;
    int32_t shift;  // log2(capacity): the capacity is always a power of two
    int64_t size;
    int64_t used;  // live entries plus deleted markers
    HNODE **A;
    uint64_t (*hash_func) (hkey_t);
    int (*comp) (hkey_t, hkey_t);
    value_t (*update) (value_t, value_t);
    HNODE deleted;  // special marker for a deleted entry 
} HTABLE;

typedef struct HITER {
    const HTABLE *h;
    int64_t curr;
} HITER;

/* Ideas for later, not implemented:

   void hash_prefix_filter(htable_prefix_t *h, bool (*filter)(hkey_t, value_t));
   put_many
   merge
   shrink
   smoother expansion instead of stop-the-world

   https://en.wikipedia.org/wiki/Hash_table
*/

static inline int64_t GLUE3(hash_, prefix, _roundup_pow2) (int64_t x) {
    /* smallest power of two >= x, for x >= 1; unsigned so that the shifts and the
       final increment cannot overflow a signed value */
    uint64_t v = (uint64_t) x - 1;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return (int64_t) v;
}

/* htable_prefix_t* hash_prefix_init(int64_t expected_size); 

   allocates and initializes a hash table.  This includes
   
   - allocating the htable_prefix_t.
   - the initial allocation of the the hash table itself.
   - setting the hash fucntion if hash.h recognizes the hkey_t type.
   - setting the comp function if comp.h recognizes teh hkey_t type.

*/

#define _unused(x) ((void)(x))

static inline HTABLE *GLUE3(hash_, prefix, _init) (int64_t expected_size) {
    /* the scaling below must not overflow in either direction, and the capacity (a power of
       two) must fit in an int64_t; no allocation of that many slots could succeed anyway */
    if (expected_size > (INT64_C(1) << 62) / 4 * 3) {
        return NULL;
    }
    if (expected_size < 0) {
        expected_size = 0;
    }
    HTABLE *h = malloc(sizeof(HTABLE));
    if (h == NULL) {
        return NULL;
    }

    expected_size = expected_size / 3 * 4 + expected_size % 3 * 4 / 3;   // expected_size / LOAD_FACTOR
    h->capacity = expected_size <= 16 ? 16 : GLUE3(hash_, prefix, _roundup_pow2) (expected_size);
    h->shift = 0;
    while (((int64_t) 1 << h->shift) < h->capacity) {
        h->shift++;
    }
    h->size = 0;
    h->used = 0;
    memset(&(h->deleted), 0, sizeof(h->deleted));
    h->A = calloc(h->capacity, sizeof(HNODE *));   /* all-bits-zero is a NULL pointer */
    if (h->A == NULL) {
        free(h);
        return NULL;
    }
    hkey_t temp;
    h->hash_func = DEFAULT_HASH(temp);
    h->comp = DEFAULT_COMP_TYPE(temp);
    _unused(temp);
    h->update = NULL;

    return h;
}

/* void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (hkey_t));

   The hash table needs a hash fuction which maps hkey_t to uint64_t.  hash.h
   will reconginze many basic types and provide a hash function.  These include
   int32_t, int, float, double, char*.  For more complicated types, the user
   must use write their own and use this routine to tell the hash table to 
   use it.    
*/
static inline void GLUE3(hash_, prefix, _set_hash) (HTABLE *h, uint64_t (*hash_func) (hkey_t)) {
    h->hash_func = hash_func;
}

/* void hash_prefix_set_comp(htable_prefix_t *h, int (*comp)(hkey_t, hkey_t));

   If no comp function is given, it is assumed the hash value means
   the same hkey_t.  If hash collisions with different hkey_ts are
   possible, a further compare function function can be set.  If should 
   return 0 if the two hkey_t are the same, and non-zero if different.

   Note: if hkey_t is char*, strcmp is used.  
*/

static inline void GLUE3(hash_, prefix, _set_comp) (HTABLE *h, int (*comp) (hkey_t, hkey_t)) {
    h->comp = comp;
}

/* void hash_prefix_set_update(htable_prefix_t *h, value_t (*update)(value_t, value_t));

   When putting a hkey_t/value_t pair into a hash table where the hkey_t already 
   exists, the default is overwrite the previous value_t with the this new one. 
   Using this routine, you can set the an update routine which combines the 
   previous value_t with the new value_t.  

   For example, if value_t is int, the following update function
  
     int update(int previous, int current) {
       return previous + current;
     }
 
   would keep the sum the values in the hash table.  

   Another example, value_t is char* and you want to free it 
   before overwritting. 

     char* update(char* previous, char* current) {
        free(previous);
        return current;
     }
*/
static inline void GLUE3(hash_, prefix, _set_update) (HTABLE *h, value_t (*update) (value_t, value_t)) {
    h->update = update;
}

/* size_t hash_prefix_size(const htable_prefix_t *h);

   Returns the number of unique hkey_ts inserted into the hash table. 
*/
static inline size_t GLUE3(hash_, prefix, _size) (const HTABLE *h) {
    return (size_t) h->size;
}

/* size_t hash_prefix_capacity(const htable_prefix_t *h);

   Returns the allocated size of the hash table.  

   Once this is 75% filled, it is automatically doubled. 
*/
static inline size_t GLUE3(hash_, prefix, _capacity) (const HTABLE *h) {
    return (size_t) h->capacity;
}

/* void hash_prefix_destroy(htable_prefix_t **h_ptr);

   Deallocates the hash table and resets all of the internal values of
   the htable_prefix_t.  Then the htable_prefix_t itself is
   deallocated and the callers pointer is set to NULL.  This avoids
   double frees.

   Note: this routine does not know how to deallocate or free the 
   hkey_ts and/or value_ts stored in the hash table.  If these are
   pointers to structs, the caller should first iterate through the
   hash table and free them appropriately. 
*/
static inline void GLUE3(hash_, prefix, _destroy) (HTABLE **h_ptr) {
    if (h_ptr == NULL) {
        return;
    }
    HTABLE *h = *h_ptr;
    if (h == NULL) {
        return;
    }

    for (int64_t i = 0; i < h->capacity; i++) {
        if (h->A[i] != &(h->deleted)) {
            free(h->A[i]);
        }
    }
    free(h->A);

    h->capacity = 0;
    h->size = 0;
    h->A = NULL;
    h->hash_func = NULL;
    h->comp = NULL;
    h->update = NULL;

    free(h);
    *h_ptr = NULL;
}

/* int32_t hash_prefix_rehash(htable_prefix_t *h);

   This routine doubles the capacity of a hash table and reinserts all
   of the entries in the new table.
*/
static inline int32_t GLUE3(hash_, prefix, _rehash) (HTABLE *h) {
    if (h == NULL) {
        return -1;
    }
    int64_t new_capacity = (h->size > LOAD_FACTOR * h->capacity / 2) ? 2 * h->capacity : h->capacity;
    HNODE **new_A = calloc(new_capacity, sizeof(HNODE *));
    if (new_A == NULL) {
        return -1;
    }
    const int32_t new_shift = h->shift + (new_capacity != h->capacity);

    const uint64_t mask = new_capacity - UINT64_C(1);

    h->size = 0;
    for (int64_t i = 0; i < h->capacity; i++) {
        HNODE *n = h->A[i];
        if (n == NULL || n == &(h->deleted)) {
            continue;
        }
        uint64_t base = n->hash & mask;
        uint64_t step = ((n->hash >> new_shift) & mask) | UINT64_C(1);

        for (uint64_t pos = base;; pos = (pos + step) & mask) {
            if (new_A[pos] == NULL) {
                new_A[pos] = n;
                break;
            }
        }
        h->size++;
    }

    free(h->A);
    h->A = new_A;
    h->capacity = new_capacity;
    h->shift = new_shift;
    h->used = h->size;
    return 0;
}

/* int32_t hash_prefix_put(htable_prefix_t *h, hkey_t key, value_t value) 

   Insert a key/value pair into the hash table.  

   What if the key is already in the table?

   1) If the user has provided a comp function (via
      hash_prefix_set_comp) then that is used to double check on hash
      value collisions.

   2) If the user has provided an update function (via
      hash_prefix_set_update), then the current value is combined with
      the new value.  If not update function is avaiable, the new
      value replaces the current value.

   return value:
     -1 => error
     0 => ok
*/

static inline int32_t GLUE3(hash_, prefix, _put) (HTABLE *h, hkey_t key, value_t value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    const uint64_t hash = hash_mix64(h->hash_func(key));
    const uint64_t mask = h->capacity - UINT64_C(1);
    const uint64_t base = hash & mask;
    const uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

    uint64_t first_empty = UINT64_MAX;

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == &(h->deleted)) {
            if (first_empty == UINT64_MAX) {
                first_empty = pos;
            }
            continue;
        }
      
        if (h->A[pos] == NULL) {
            if (first_empty == UINT64_MAX) {
                first_empty = pos;
            }
            /* probes stop at an empty slot, so the last one must never be used up */
            if (h->A[first_empty] == NULL && (uint64_t) h->used + 1 >= (uint64_t) h->capacity) {
                return -1;
            }
            HNODE *n = malloc(sizeof(HNODE));
            if (n == NULL) {
                return -1;
            }
            n->hash = hash;
            n->key = key;
            n->value = value;
            if (h->A[first_empty] == NULL) {
                h->used++;
            }
            h->size++;
            h->A[first_empty] = n;
            break;
        }

        if (h->A[pos]->hash == hash) {
            if (h->comp == NULL || h->comp(key, h->A[pos]->key) == 0) {
                h->A[pos]->value = h->update == NULL ? value : h->update(h->A[pos]->value, value);
                break;
            }
        }
    }

    if (h->used > LOAD_FACTOR * h->capacity) {
        /* best effort: the entry is already stored, and the next put tries again */
        (void) GLUE3(hash_, prefix, _rehash) (h);
    }
    return 0;
}

/* int32_T hash_prefix_get(htable_prefix_t *h, hkey_t key, value_t *value) 

   Recovers a value from a hash table.  *value can be NULL if the user 
   only wants to know if the key is in the table.  

   Return Value:
     -1 => an error
     0 => key not found
     1 => key found, *value set to the corresponding value.
*/

static inline int32_t GLUE3(hash_, prefix, _get) (const HTABLE *h, hkey_t key, value_t *value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    uint64_t hash = hash_mix64(h->hash_func(key));
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == NULL) {
            return 0;
        } else {
            if (h->A[pos] != &(h->deleted) && h->A[pos]->hash == hash) {
                if (h->comp == NULL || h->comp(key, h->A[pos]->key) == 0) {
                    if (value != NULL) {
                        *value = h->A[pos]->value;
                    }
                    return 1;
                }
            }
        }
    }
}

/* hash_prefix_remove(htable_prefix_t *h, hkey_t key, value_t *value) 

   
*/

static inline int32_t GLUE3(hash_, prefix, _remove) (HTABLE *h, hkey_t key, value_t *value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    uint64_t hash = hash_mix64(h->hash_func(key));
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == NULL) {
            return 0;
        } else {
            if (h->A[pos] != &(h->deleted) && h->A[pos]->hash == hash) {
                if (h->comp == NULL || h->comp(key, h->A[pos]->key) == 0) {
                    if (value != NULL) {
                        *value = h->A[pos]->value;
                    }
                    free(h->A[pos]);
                    h->A[pos] = &(h->deleted);
                    h->size--;
                    return 1;
                }
            }
        }
    }
}

static inline int32_t GLUE3(hash_, prefix, _next) (HITER **iter_ptr, hkey_t *key, value_t *value) {
    HITER *iter = *iter_ptr;
    const HTABLE *h = iter->h;
    int64_t curr = iter->curr;

    while (1) {
        if (curr == h->capacity) {
            iter->h = NULL;
            iter->curr = -1;
            free(iter);
            *iter_ptr = NULL;
            return 1;
        }

        if (h->A[curr] != NULL && h->A[curr] != &(h->deleted)) {
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

static inline int32_t GLUE3(hash_, prefix, _first) (const HTABLE *h, HITER **iter_ptr, hkey_t *key, value_t *value) {
    HITER *iter = malloc(sizeof(HITER));
    if (iter == NULL) {
        *iter_ptr = NULL;
        return -1;
    }
    iter->h = h;
    iter->curr = 0;
    *iter_ptr = iter;

    return GLUE3(hash_, prefix, _next) (iter_ptr, key, value);
}

static inline void GLUE3(hash_, prefix, _apply) (HTABLE *h, value_t (*f) (hkey_t, value_t)) {
    for (int64_t idx = 0; idx < h->capacity; idx++) {
        HNODE *a = h->A[idx];
        if (a != NULL && a != &(h->deleted)) {
            a->value = f(a->key, a->value);
        }
    }
}

static inline void GLUE3(hash_, prefix, _apply_r) (HTABLE *h, value_t (*f) (hkey_t, value_t, void *), void *arg) {
    for (int64_t idx = 0; idx < h->capacity; idx++) {
        HNODE *a = h->A[idx];
        if (a != NULL && a != &(h->deleted)) {
            a->value = f(a->key, a->value, arg);
        }
    }
}

static inline HTABLE *GLUE3(hash_, prefix, _clone) (HTABLE *h) {
  HTABLE *out = GLUE3(hash_, prefix, _init) (h->size);
  if (out == NULL) {
    return NULL;
  }

  out->hash_func = h->hash_func;
  out->comp = h->comp;
  out->update = h->update;

  for (int64_t i = 0; i < h->capacity; i++) {
    if (h->A[i] != NULL && h->A[i] != &(h->deleted)) {
      int32_t rc = GLUE3(hash_, prefix, _put) (out, h->A[i]->key, h->A[i]->value);
      if (rc != 0) {
        GLUE3(hash_, prefix, _destroy) (&out);
        return NULL;
      }
    }
  }

  return out;
}

#undef HNODE
#undef HTABLE
#undef HITER
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting) are
   removed so that they do not leak into the includer */
#undef _unused
#undef LOAD_FACTOR
