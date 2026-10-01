#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
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

#define PHTABLE GLUE3(phtable_, prefix, _t)
#define PHNODE GLUE3(phnode_, prefix, _t)

#define LOAD_FACTOR (0.75)

/* Basic structs used in the header file. 
 
*/
typedef struct PHNODE {
    uint64_t hash;
    hkey_t key;
    value_t value;
} PHNODE;

typedef struct PHTABLE {
    int64_t capacity;
    int32_t shift;  // log2(capacity): the capacity is always a power of two
    int64_t size;
    int64_t used;  // live entries plus deleted markers
    PHNODE **A;
    /* atomic so that it can be read without holding the lock: the hash of a key
       does not depend on the table, so it is computed before locking */
    _Atomic(uint64_t (*) (hkey_t)) hash_func;
    int (*comp) (hkey_t, hkey_t);
    value_t (*update) (value_t, value_t);
    PHNODE deleted;  // special marker for a deleted entry
    pthread_rwlock_t rwlock;
} PHTABLE;

/* 
   void hash_prefix_filter(htable_prefix_t *h, bool (*filter)(hkey_t, value_t));
   void Hash_prefix_apply_r(htable_prefix_t *h, value_t (*apply_r) (hkey_t, value_t, void*), void *arg);
   htable_prefix_t* hash_prefix_duplicate(const htable_prefix_t *h)
   
   put_many
   merge

   shrink?
   smoother expansion instead of stop-the-world
 
https://en.wikipedia.org/wiki/Hash_table
*/

static inline int64_t GLUE3(phash_, prefix, _roundup_pow2) (int64_t x) {
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
   - setting the hash fucntion if hash.h recognizes the hkey_t type.
   - setting the comp function if comp.h recognizes teh hkey_t type.

*/

#define _unused(x) ((void)(x))

static inline PHTABLE *GLUE3(phash_, prefix, _init) (int64_t expected_size) {
    PHTABLE *h = malloc(sizeof(PHTABLE));
    if (h == NULL) {
        return NULL;
    }

    expected_size = expected_size / 3 * 4 + expected_size % 3 * 4 / 3;   // expected_size / LOAD_FACTOR
    h->capacity = expected_size <= 16 ? 16 : GLUE3(phash_, prefix, _roundup_pow2) (expected_size);
    h->shift = 0;
    while (((int64_t) 1 << h->shift) < h->capacity) {
        h->shift++;
    }
    h->size = 0;
    h->used = 0;
    memset(&(h->deleted), 0, sizeof(h->deleted));
    h->A = calloc(h->capacity, sizeof(PHNODE *));   /* all-bits-zero is a NULL pointer */
    if (h->A == NULL) {
        free(h);
        return NULL;
    }
    hkey_t temp;
    h->hash_func = DEFAULT_HASH(temp);
    h->comp = DEFAULT_COMP_TYPE(temp);
    _unused(temp);
    h->update = NULL;

    if (pthread_rwlock_init(&(h->rwlock), NULL) != 0) {
        free(h->A);
        free(h);
        return NULL;
    }

    return h;
}

/* void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (hkey_t));

   The hash table needs a hash fuction which maps hkey_t to uint64_t.  hash.h
   will reconginze many basic types and provide a hash function.  These include
   int32_t, int, float, double, char*.  For more complicated types, the user
   must use write their own and use this routine to tell the hash table to 
   use it.    
*/
static inline void GLUE3(phash_, prefix, _set_hash) (PHTABLE *h, uint64_t (*hash_func) (hkey_t)) {
    atomic_store_explicit(&h->hash_func, hash_func, memory_order_release);
}

/* void hash_prefix_set_comp(htable_prefix_t *h, int (*comp)(hkey_t, hkey_t));

   If no comp function is given, it is assumed the hash value means
   the same hkey_t.  If hash collisions with different hkey_ts are
   possible, a further compare function function can be set.  If should 
   return 0 if the two hkey_t are the same, and non-zero if different.

   Note: if hkey_t is char*, strcmp is used.  
*/

static inline void GLUE3(phash_, prefix, _set_comp) (PHTABLE *h, int (*comp) (hkey_t, hkey_t)) {
    pthread_rwlock_wrlock(&(h->rwlock));
    h->comp = comp;
    pthread_rwlock_unlock(&(h->rwlock));
}

/* void hash_prefix_set_update(htable_prefix_t *h, value_t (*update)(value_t, value_t));

   When putting a hkey_t/value_t pair into a hash table where the hkey_t already 
   exists, the default is overwrite the previous value_t with the this new one. 
   Using the routine, you can set the an update routine which combines the 
   previous value_t with the new value_t.  

   For example, if value_t is int, the following update function
  
     int update(int previous, int current) {
       return previous + current;
     }
 
   would keep the sum the values in the hash table. 
*/
static inline void GLUE3(phash_, prefix, _set_update) (PHTABLE *h, value_t (*update) (value_t, value_t)) {
    pthread_rwlock_wrlock(&(h->rwlock));
    h->update = update;
    pthread_rwlock_unlock(&(h->rwlock));
}

/* int64_t hash_prefix_get_size(const htable_prefix_t *h);

   Returns the number of unique hkey_ts inserted into the hash table. 
*/
static inline int64_t GLUE3(phash_, prefix, _get_size) (PHTABLE *h) {
    pthread_rwlock_rdlock(&(h->rwlock));
    int64_t rv = h->size;
    pthread_rwlock_unlock(&(h->rwlock));
    return rv;
}

/* int64_t hash_prefix_get_capacity(const htable_prefix_t *h);

   Returns the allocated size of the hash table.  

   Once this is 75% filled, it is automatically doubled. 
*/
static inline int64_t GLUE3(phash_, prefix, _get_capacity) (PHTABLE *h) {
    pthread_rwlock_rdlock(&(h->rwlock));
    int64_t rv =  h->capacity;
    pthread_rwlock_unlock(&(h->rwlock));
    return rv;
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
static inline void GLUE3(phash_, prefix, _destroy) (PHTABLE **h_ptr) {
    PHTABLE *h = *h_ptr;
    if (h == NULL) {
        return;
    }
    pthread_rwlock_wrlock(&(h->rwlock));
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

    pthread_rwlock_unlock(&(h->rwlock));
    pthread_rwlock_destroy(&(h->rwlock));

    free(h);
    *h_ptr = NULL;
}

/* int32_t hash_prefix_rehash(htable_prefix_t *h);

   This routine doubles the capacity of a hash table and reinserts all
   of the entries in the new table.
*/
static inline int32_t GLUE3(phash_, prefix, _rehash) (PHTABLE *h) {
    if (h == NULL) {
        return -1;
    }

    int64_t new_capacity = (h->size > LOAD_FACTOR * h->capacity / 2) ? 2 * h->capacity : h->capacity;
    PHNODE **new_A = calloc(new_capacity, sizeof(PHNODE *));
    if (new_A == NULL) {
        return -1;
    }
    const int32_t new_shift = h->shift + (new_capacity != h->capacity);

    const uint64_t mask = new_capacity - UINT64_C(1);

    for (int64_t i = 0; i < h->capacity; i++) {
        PHNODE *n = h->A[i];
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

/* Inserts key/value, or combines value with the current value if key is
   already present.  The caller holds the write lock and has computed
   hash = hash_mix64(hash_func(key)) before taking it.  An existing value
   becomes update(old, value), or just value if update is NULL. */
static inline int32_t GLUE3(phash_, prefix, _put_locked) (PHTABLE *h, uint64_t hash, hkey_t key,
                                                   value_t value,
                                                   value_t (*update) (value_t, value_t)) {
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
            PHNODE *n = malloc(sizeof(PHNODE));
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
                h->A[pos]->value = update == NULL ? value : update(h->A[pos]->value, value);
                break;
            }
        }
    }

    if (h->used > LOAD_FACTOR * h->capacity) {
        return GLUE3(phash_, prefix, _rehash) (h);
    }
    return 0;
}

static inline int32_t GLUE3(phash_, prefix, _put) (PHTABLE *h, hkey_t key, value_t value) {
    if (h == NULL) {
        return -1;
    }
    uint64_t (*hash_func) (hkey_t) = atomic_load_explicit(&h->hash_func, memory_order_acquire);
    if (hash_func == NULL) {
        return -1;
    }
    const uint64_t hash = hash_mix64(hash_func(key));

    pthread_rwlock_wrlock(&(h->rwlock));
    int32_t rc = GLUE3(phash_, prefix, _put_locked) (h, hash, key, value, h->update);
    pthread_rwlock_unlock(&(h->rwlock));
    return rc;
}

static inline int32_t GLUE3(phash_, prefix, _atomic_update) (PHTABLE *h, hkey_t key, value_t value,
					       value_t (*update) (value_t, value_t)) {
    if (h == NULL || update == NULL) {
        return -1;
    }
    uint64_t (*hash_func) (hkey_t) = atomic_load_explicit(&h->hash_func, memory_order_acquire);
    if (hash_func == NULL) {
        return -1;
    }
    const uint64_t hash = hash_mix64(hash_func(key));

    pthread_rwlock_wrlock(&(h->rwlock));
    int32_t rc = GLUE3(phash_, prefix, _put_locked) (h, hash, key, value, update);
    pthread_rwlock_unlock(&(h->rwlock));
    return rc;
}

/* int32_T hash_prefix_get(htable_prefix_t *h, hkey_t key, value_t *value) 

   Recovers a value from a hash table.  *value can be NULL if the user 
   only wants to know if the key is in the table.  

   Return Value:
     -1 => an error
     0 => key not found
     1 => key found, *value set to the corresponding value.
*/

static inline int32_t GLUE3(phash_, prefix, _get) (PHTABLE *h, hkey_t key, value_t *value) {
    if (h == NULL) {
        return -1;
    }
    uint64_t (*hash_func) (hkey_t) = atomic_load_explicit(&h->hash_func, memory_order_acquire);
    if (hash_func == NULL) {
        return -1;
    }
    uint64_t hash = hash_mix64(hash_func(key));

    pthread_rwlock_rdlock(&(h->rwlock));
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == NULL) {
	    pthread_rwlock_unlock(&(h->rwlock));
            return 0;
        } else {
	    if (h->A[pos] != &(h->deleted) && h->A[pos]->hash == hash) {
                if (h->comp == NULL || h->comp(key, h->A[pos]->key) == 0) {
                    if (value != NULL) {
                        *value = h->A[pos]->value;
                    }
		    pthread_rwlock_unlock(&(h->rwlock));	
                    return 1;
                }
            }
        }
    }
}

/* hash_prefix_remove(htable_prefix_t *h, hkey_t key, value_t *value) 

   
*/

static inline int32_t GLUE3(phash_, prefix, _remove) (PHTABLE *h, hkey_t key, value_t *value) {
    if (h == NULL) {
        return -1;
    }
    uint64_t (*hash_func) (hkey_t) = atomic_load_explicit(&h->hash_func, memory_order_acquire);
    if (hash_func == NULL) {
        return -1;
    }
    uint64_t hash = hash_mix64(hash_func(key));

    pthread_rwlock_wrlock(&(h->rwlock));
    uint64_t mask = h->capacity - UINT64_C(1);
    uint64_t base = hash & mask;
    uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

    for (uint64_t pos = base;; pos = (pos + step) & mask) {
        if (h->A[pos] == NULL) {
	    pthread_rwlock_unlock(&(h->rwlock));
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
		    pthread_rwlock_unlock(&(h->rwlock));
                    return 1;
                }
            }
        }
    }
    assert(0);
}

#undef PHNODE
#undef PHTABLE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting) are
   removed so that they do not leak into the includer */
#undef _unused
#undef LOAD_FACTOR
