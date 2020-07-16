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

/* Basic structs used in the header file. 
 
*/
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
    value_t (*update) (value_t, value_t);
    HNODE deleted;  // special marker for a deleted entry 
} HTABLE;

typedef struct HITER {
    const HTABLE *h;
    uint64_t curr;
} HITER;

/* 
   void hash_prefix_filter(htable_prefix_t *h, bool (*filter)(key_t, value_t));
   void Hash_prefix_apply_r(htable_prefix_t *h, value_t (*apply_r) (key_t, value_t, void*), void *arg);
   htable_prefix_t* hash_prefix_duplicate(const htable_prefix_t *h)
   
   put_many
   merge

   shrink?
   smoother expansion instead of stop-the-world
 
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

/* htable_prefix_t* hash_prefix_init(int64_t expected_size); 

   allocates and initializes a hash table.  This includes
   
   - allocating the htable_prefix_t.
   - the initial allocation of the the hash table itself.
   - setting the hash fucntion if hash.h recognizes the key_t type.
   - setting the comp function if comp.h recognizes teh key_t type.

*/

#define _unused(x) ((void)(x))

HTABLE *GLUE3(hash_, prefix, _init) (int64_t expected_size) {
    HTABLE *h = malloc(sizeof(HTABLE));
    if (h == NULL) {
        return NULL;
    }

    expected_size /= LOAD_FACTOR;
    h->capacity = expected_size <= 16 ? 16 : GLUE3(hash_, prefix, _roundup_pow2) (expected_size);
    h->size = 0;
    h->A = malloc(h->capacity * sizeof(HNODE *));
    if (h->A == NULL) {
        return NULL;
    }
    for (int64_t i = 0; i < h->capacity; i++) {
        h->A[i] = NULL;
    }
    key_t temp;
    h->hash_func = DEFAULT_HASH(temp);
    h->comp = DEFAULT_COMP_TYPE(temp);
    _unused(temp);
    h->update = NULL;

    return h;
}

/* void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (key_t));

   The hash table needs a hash fuction which maps key_t to uint64_t.  hash.h
   will reconginze many basic types and provide a hash function.  These include
   int32_t, int, float, double, char*.  For more complicated types, the user
   must use write their own and use this routine to tell the hash table to 
   use it.    
*/
void GLUE3(hash_, prefix, _set_hash) (HTABLE *h, uint64_t (*hash_func) (key_t)) {
    h->hash_func = hash_func;
}

/* void hash_prefix_set_comp(htable_prefix_t *h, int (*comp)(key_t, key_t));

   If no comp function is given, it is assumed the hash value means
   the same key_t.  If hash collisions with different key_ts are
   possible, a further compare function function can be set.  If should 
   return 0 if the two key_t are the same, and non-zero if different.

   Note: if key_t is char*, strcmp is used.  
*/

void GLUE3(hash_, prefix, _set_comp) (HTABLE *h, int (*comp) (key_t, key_t)) {
    h->comp = comp;
}

/* void hash_prefix_set_update(htable_prefix_t *h, value_t (*update)(value_t, value_t));

   When putting a key_t/value_t pair into a hash table where the key_t already 
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
void GLUE3(hash_, prefix, _set_update) (HTABLE *h, value_t (*update) (value_t, value_t)) {
    h->update = update;
}

/* int64_t hash_prefix_get_size(const htable_prefix_t *h);

   Returns the number of unique key_ts inserted into the hash table. 
*/
int64_t GLUE3(hash_, prefix, _get_size) (const HTABLE *h) {
    return h->size;
}

/* int64_t hash_prefix_get_capacity(const htable_prefix_t *h);

   Returns the allocated size of the hash table.  

   Once this is 75% filled, it is automatically doubled. 
*/
int64_t GLUE3(hash_, prefix, _get_capacity) (const HTABLE *h) {
    return h->capacity;
}

/* void hash_prefix_destroy(htable_prefix_t **h_ptr);

   Deallocates the hash table and resets all of the internal values of
   the htable_prefix_t.  Then the htable_prefix_t itself is
   deallocated and the callers pointer is set to NULL.  This avoids
   double frees.

   Note: this routine does not know how to deallocate or free the 
   key_ts and/or value_ts stored in the hash table.  If these are
   pointers to structs, the caller should first iterate through the
   hash table and free them appropriately. 
*/
void GLUE3(hash_, prefix, _destroy) (HTABLE **h_ptr) {
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
    h_ptr = NULL;
}

/* int32_t hash_prefix_rehash(htable_prefix_t *h);

   This routine doubles the capacity of a hash table and reinserts all
   of the entries in the new table.
*/
int32_t GLUE3(hash_, prefix, _rehash) (HTABLE *h) {
    if (h == NULL) {
        return -1;
    }
    int64_t new_capacity = 2 * h->capacity;
    HNODE **new_A = malloc(new_capacity * sizeof(HNODE *));
    if (new_A == NULL) {
        return -1;
    }
    for (int64_t i = 0; i < new_capacity; i++) {
        new_A[i] = NULL;
    }

    const uint64_t mask = new_capacity - UINT64_C(1);

    h->size = 0;
    for (int64_t i = 0; i < h->capacity; i++) {
        HNODE *n = h->A[i];
        if (n == NULL || n == &(h->deleted)) {
            continue;
        }
        uint64_t base = n->hash & mask;
        uint64_t step = ((n->hash / new_capacity) & mask) | UINT64_C(1);

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
    return 0;
}

/* int32_t hash_prefix_put(htable_prefix_t *h, key_t key, value_t value) 

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

int32_t GLUE3(hash_, prefix, _put) (HTABLE *h, key_t key, value_t value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    const uint64_t hash = h->hash_func(key);
    const uint64_t mask = h->capacity - UINT64_C(1);
    const uint64_t base = hash & mask;
    const uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);

    uint64_t first_empty = -1;

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == &(h->deleted)) {
	    if (first_empty == -1) {
	        first_empty = pos;
	    }
	    continue;
        }
      
        if (h->A[pos] == NULL) {
	    if (first_empty == -1) {
	        first_empty = pos;
	    }
            HNODE *n = malloc(sizeof(HNODE));
            if (n == NULL) {
                return -1;
            }
            n->hash = hash;
            n->key = key;
            n->value = value;
	    if (h->A[first_empty] == NULL) {
	        h->size++;
	    }
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

    if (h->size > LOAD_FACTOR * h->capacity) {
        int32_t rc = GLUE3(hash_, prefix, _rehash) (h);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

/* int32_T hash_prefix_get(htable_prefix_t *h, key_t key, value_t *value) 

   Recovers a value from a hash table.  *value can be NULL if the user 
   only wants to know if the key is in the table.  

   Return Value:
     -1 => an error
     0 => key not found
     1 => key found, *value set to the corresponding value.
*/

int32_t GLUE3(hash_, prefix, _get) (const HTABLE *h, key_t key, value_t *value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    uint64_t hash = h->hash_func(key);
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);

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

/* hash_prefix_remove(htable_prefix_t *h, key_t key, value_t *value) 

   
*/

int32_t GLUE3(hash_, prefix, _remove) (HTABLE *h, key_t key, value_t *value) {
    if (h == NULL || h->hash_func == NULL) {
        return -1;
    }
    uint64_t hash = h->hash_func(key);
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash / h->capacity) & mask) | UINT64_C(1);

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == NULL) {
            return 0;
        } else {
            if (h->A[pos]->hash == hash) {
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

int32_t GLUE3(hash_, prefix, _first) (const HTABLE *h, HITER **iter_ptr, key_t *key, value_t *value) {
    HITER *iter = malloc(sizeof(HITER));
    iter->h = h;
    iter->curr = 0;
    *iter_ptr = iter;

    return GLUE3(hash_, prefix, _next) (iter_ptr, key, value);
}

void GLUE3(hash_, prefix, _apply) (HTABLE *h, value_t (*f) (key_t, value_t)) {
    for (int64_t idx = 0; idx < h->capacity; idx++) {
        HNODE *a = h->A[idx];
        if (a != NULL && a != &(h->deleted)) {
            a->value = f(a->key, a->value);
        }
    }
}

void GLUE3(hash_, prefix, _apply_r) (HTABLE *h, value_t (*f) (key_t, value_t, void *), void *arg) {
    for (int64_t idx = 0; idx < h->capacity; idx++) {
        HNODE *a = h->A[idx];
        if (a != NULL && a != &(h->deleted)) {
            a->value = f(a->key, a->value, arg);
        }
    }
}

HTABLE *GLUE3(hash_, prefix, _clone) (HTABLE *h) {
  HTABLE *out = GLUE3(hash_, prefix, _init) (h->size);
  if (out == NULL) {
    return NULL;
  }

  out->hash_func = h->hash_func;
  out->comp = h->comp;
  out->update = h->update;

  for (int64_t i = 0; i < h->capacity; i++) {
    if (h->A[i] != NULL) {
      int32_t rc = GLUE3(hash_, prefix, _put) (out, h->A[i]->key, h->A[i]->value);
      if (rc != 0) {
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
