#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "cs_assert.h"
#include "cs_private.h"
#include "hash.h"
#include "comp.h"
#include "key_hook.h"

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
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

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

/* The nodes are allocated from a pool (pool.h).  The pool is not thread safe,
   and does not need to be here: nodes are taken and given back only while the
   write lock is held, and the pool is destroyed with the table.  The pool
   header defines and undefines the same helper macros as this one, so they are
   removed here and defined again after it. */
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
#pragma push_macro("data_t")
#undef data_t
#define data_t PHNODE
#define pool_label GLUE(phn_, prefix)
#include "pool.h"
#undef pool_label
#undef data_t
#pragma pop_macro("data_t")
#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define PHPOOL GLUE3(pool_phn_, prefix, _t)

/* capacity and size are atomic so that phash_prefix_size() and
   phash_prefix_capacity() can read them without the lock.  They are changed
   only while the write lock is held, so inside the lock relaxed loads and
   stores (CS_LOAD / CS_STORE) are enough. */
#define CS_LOAD(x) atomic_load_explicit(&(x), memory_order_relaxed)
#define CS_STORE(x, v) atomic_store_explicit(&(x), (v), memory_order_relaxed)

typedef struct PHTABLE {
  _Atomic int64_t capacity;
  int32_t shift; // log2(capacity): the capacity is always a power of two
  _Atomic int64_t size;
  int64_t used; // live entries plus deleted markers
  PHNODE **A;
  PHPOOL *pool; // the space of the nodes; created by the first put
  /* atomic so that it can be read without holding the lock: the hash of a key
     does not depend on the table, so it is computed before locking */
  _Atomic(uint64_t (*)(hkey_t)) hash_func;
  int (*comp)(hkey_t, hkey_t);
  value_t (*update)(value_t, value_t);
  PHNODE deleted; // special marker for a deleted entry
  pthread_rwlock_t rwlock;
} PHTABLE;

/* Ideas for later, not implemented (hash_table.h has some of them, but here
   they would have to hold the lock for the whole walk):

   iteration, apply, clone
   filter (keep the entries for which a predicate is true)
   put_many
   merge
   shrink
   smoother expansion instead of stop-the-world

   https://en.wikipedia.org/wiki/Hash_table
*/

CS_PRIVATE static inline int64_t GLUE3(phash_, prefix,
                                       _roundup_pow2)(int64_t x) {
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

/* phtable_prefix_t* phash_prefix_init(int64_t expected_size);

   allocates and initializes a hash table.  This includes

   - allocating the phtable_prefix_t.
   - the initial allocation of the hash table itself.
   - setting the hash function if hash.h recognizes the hkey_t type.
   - setting the comp function if comp.h recognizes the hkey_t type.

*/

#define _unused(x) ((void) (x))

static inline PHTABLE *GLUE3(phash_, prefix, _init)(int64_t expected_size) {
  /* the scaling below must not overflow in either direction, and the capacity
     (a power of two) must fit in an int64_t; no allocation of that many slots
     could succeed anyway */
  if (expected_size > (INT64_C(1) << 62) / 4 * 3) {
    return NULL;
  }
  if (expected_size < 0) {
    expected_size = 0;
  }
  PHTABLE *h = malloc(sizeof(PHTABLE));
  if (h == NULL) {
    return NULL;
  }

  expected_size = expected_size / 3 * 4 +
                  expected_size % 3 * 4 / 3; // expected_size / LOAD_FACTOR
  const int64_t capacity =
      expected_size <= 16 ? 16
                          : GLUE3(phash_, prefix, _roundup_pow2)(expected_size);
  atomic_init(&h->capacity, capacity);
  h->shift = 0;
  while (((int64_t) 1 << h->shift) < capacity) {
    h->shift++;
  }
  atomic_init(&h->size, 0);
  h->used = 0;
  h->pool = NULL;
  memset(&(h->deleted), 0, sizeof(h->deleted));
  h->A =
      calloc(capacity, sizeof(PHNODE *)); /* all-bits-zero is a NULL pointer */
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

/* int32_t phash_prefix_set_hash(phtable_prefix_t *h, uint64_t (*hash_func)
   (hkey_t));

   The hash table needs a hash function which maps hkey_t to uint64_t.  hash.h
   will recognize many basic types and provide a hash function.  These include
   int32_t, int, float, double, char*.  For more complicated types, the user
   must write their own and use this routine to tell the hash table to
   use it.

   It must be called while the table is empty: every entry is stored where its
   hash says, so after a different function is set the entries already stored
   cannot be found.  Set it before the table is shared with other threads: the
   hash of a key is computed before the lock is taken, so a put which is
   already running can still use the old function.

   return value:
     -1 => error (h is NULL, or the table has entries; nothing is changed)
      0 => ok
*/
static inline int32_t
GLUE3(phash_, prefix, _set_hash)(PHTABLE *h, uint64_t (*hash_func)(hkey_t)) {
  if (h == NULL) {
    return -1;
  }
  pthread_rwlock_wrlock(&(h->rwlock));
  if (CS_LOAD(h->size) > 0) {
    pthread_rwlock_unlock(&(h->rwlock));
    return -1;
  }
  atomic_store_explicit(&h->hash_func, hash_func, memory_order_release);
  pthread_rwlock_unlock(&(h->rwlock));
  return 0;
}

/* void phash_prefix_set_comp(phtable_prefix_t *h, int (*comp)(hkey_t, hkey_t));

   If no comp function is given, it is assumed the hash value means
   the same hkey_t.  If hash collisions with different hkey_ts are
   possible, a further compare function can be set.  It should
   return 0 if the two hkey_t are the same, and non-zero if different.

   Note: if hkey_t is char*, strcmp is used.
*/

static inline void GLUE3(phash_, prefix,
                         _set_comp)(PHTABLE *h, int (*comp)(hkey_t, hkey_t)) {
  pthread_rwlock_wrlock(&(h->rwlock));
  h->comp = comp;
  pthread_rwlock_unlock(&(h->rwlock));
}

/* void phash_prefix_set_update(phtable_prefix_t *h, value_t (*update)(value_t,
   value_t));

   When putting a hkey_t/value_t pair into a hash table where the hkey_t already
   exists, the default is to overwrite the previous value_t with this new one.
   Using the routine, you can set an update routine which combines the
   previous value_t with the new value_t.

   For example, if value_t is int, the following update function

     int update(int previous, int current) {
       return previous + current;
     }

   would keep the sum the values in the hash table.
*/
static inline void GLUE3(phash_, prefix,
                         _set_update)(PHTABLE *h,
                                      value_t (*update)(value_t, value_t)) {
  pthread_rwlock_wrlock(&(h->rwlock));
  h->update = update;
  pthread_rwlock_unlock(&(h->rwlock));
}

/* size_t phash_prefix_size(const phtable_prefix_t *h);

   Returns the number of unique hkey_ts inserted into the hash table.  It does
   not take the lock: while other threads change the table, the value is the
   size at some moment during the call.
*/
static inline size_t GLUE3(phash_, prefix, _size)(const PHTABLE *h) {
  return (size_t) CS_LOAD(h->size);
}

/* size_t phash_prefix_capacity(const phtable_prefix_t *h);

   Returns the allocated size of the hash table.

   Once this is 75% filled, it is automatically doubled.  Like
   phash_prefix_size(), it does not take the lock.
*/
static inline size_t GLUE3(phash_, prefix, _capacity)(const PHTABLE *h) {
  return (size_t) CS_LOAD(h->capacity);
}

/* void phash_prefix_destroy(phtable_prefix_t **h_ptr);

   Deallocates the hash table and resets all of the internal values of
   the phtable_prefix_t.  Then the phtable_prefix_t itself is
   deallocated and the callers pointer is set to NULL.  This avoids
   double frees.

   Note: this routine does not know how to deallocate or free the
   hkey_ts and/or value_ts stored in the hash table.  If these are
   pointers to structs, the caller should first iterate through the
   hash table and free them appropriately.
*/
static inline void GLUE3(phash_, prefix, _destroy)(PHTABLE **h_ptr) {
  if (h_ptr == NULL) {
    return;
  }
  PHTABLE *h = *h_ptr;
  if (h == NULL) {
    return;
  }
  pthread_rwlock_wrlock(&(h->rwlock));
  if (h->pool != NULL) {
    GLUE3(pool_phn_, prefix, _destroy)(&h->pool); /* all the nodes at once */
  }
  free(h->A);

  CS_STORE(h->capacity, 0);
  CS_STORE(h->size, 0);
  h->A = NULL;
  h->hash_func = NULL;
  h->comp = NULL;
  h->update = NULL;

  pthread_rwlock_unlock(&(h->rwlock));
  pthread_rwlock_destroy(&(h->rwlock));

  free(h);
  *h_ptr = NULL;
}

/* int32_t phash_prefix_rehash(phtable_prefix_t *h);

   This routine doubles the capacity of a hash table and reinserts all
   of the entries in the new table.  Internal: the caller must hold the write
   lock.
*/
CS_PRIVATE static inline int32_t GLUE3(phash_, prefix, _rehash)(PHTABLE *h) {
  if (h == NULL) {
    return -1;
  }

  const int64_t size = CS_LOAD(h->size);
  const int64_t capacity = CS_LOAD(h->capacity);
  int64_t new_capacity =
      (size > LOAD_FACTOR * capacity / 2) ? 2 * capacity : capacity;
  PHNODE **new_A = calloc(new_capacity, sizeof(PHNODE *));
  if (new_A == NULL) {
    return -1;
  }
  const int32_t new_shift = h->shift + (new_capacity != capacity);

  const uint64_t mask = new_capacity - UINT64_C(1);

  for (int64_t i = 0; i < capacity; i++) {
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
  CS_STORE(h->capacity, new_capacity);
  h->shift = new_shift;
  h->used = size;
  return 0;
}

/* PHNODE *phash_prefix_node_alloc(phtable_prefix_t *h, int64_t capacity);

   Space for one node, or NULL if memory is exhausted.  The caller holds the
   write lock.  The pool is created by the first call, with room for the entries
   the initial capacity holds below the load factor; it then adds a block 1.5
   times as large each time it is full.  Space of removed entries goes back to
   the pool and is used again.  An empty table owns no pool.
*/
CS_PRIVATE static inline PHNODE *
GLUE3(phash_, prefix, _node_alloc)(PHTABLE *h, int64_t capacity) {
  if (h->pool == NULL) {
    h->pool = GLUE3(pool_phn_, prefix, _init)((size_t) (capacity / 4 * 3));
    if (h->pool == NULL) {
      return NULL;
    }
  }
  return GLUE3(pool_phn_, prefix, _get)(h->pool);
}

/* int32_t phash_prefix_put(phtable_prefix_t *h, hkey_t key, value_t value)

   Insert a key/value pair into the hash table.

   What if the key is already in the table?

   1) If the user has provided a comp function (via
      phash_prefix_set_comp) then that is used to double check on hash
      value collisions.

   2) If the user has provided an update function (via
      phash_prefix_set_update), then the current value is combined with
      the new value.  If no update function is available, the new
      value replaces the current value.

   return value:
     -1 => error (the table is unchanged)
     0 => ok, the key was not in the table and has been inserted
     1 => ok, the key was already in the table and its value was replaced
          (or combined with the new value by the update function)
*/

/* Inserts key/value, or combines value with the current value if key is
   already present.  The caller holds the write lock and has computed
   hash = hash_mix64(hash_func(key)) before taking it.  An existing value
   becomes update(old, value), or just value if update is NULL.  Returns 0 if
   the key was inserted, 1 if it was already there, -1 on error. */
CS_PRIVATE static inline int32_t
GLUE3(phash_, prefix, _put_locked)(PHTABLE *h, uint64_t hash, hkey_t key,
                                   value_t value,
                                   value_t (*update)(value_t, value_t)) {
  const int64_t capacity = CS_LOAD(h->capacity);
  const uint64_t mask = capacity - UINT64_C(1);
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
      if (h->A[first_empty] == NULL &&
          (uint64_t) h->used + 1 >= (uint64_t) capacity) {
        return -1;
      }
      PHNODE *n = GLUE3(phash_, prefix, _node_alloc)(h, capacity);
      if (n == NULL) {
        return -1;
      }
      n->hash = hash;
      n->key = key;
      n->value = value;
      if (h->A[first_empty] == NULL) {
        h->used++;
      }
      CS_STORE(h->size, CS_LOAD(h->size) + 1);
      h->A[first_empty] = n;
      break;
    }

    if (h->A[pos]->hash == hash) {
      if (CS_KEYEQ(h->comp, key, h->A[pos]->key)) {
        h->A[pos]->value =
            update == NULL ? value : update(h->A[pos]->value, value);
        return 1; /* replaced: the number of entries did not change */
      }
    }
  }

  if (h->used > LOAD_FACTOR * capacity) {
    /* best effort: the entry is already stored, and the next put tries again */
    (void) GLUE3(phash_, prefix, _rehash)(h);
  }
  return 0;
}

static inline int32_t GLUE3(phash_, prefix, _put)(PHTABLE *h, hkey_t key,
                                                  value_t value) {
  if (h == NULL) {
    return -1;
  }
  uint64_t (*hash_func)(hkey_t) =
      atomic_load_explicit(&h->hash_func, memory_order_acquire);
  if (!CS_HAVE_HASH(hash_func)) {
    return -1;
  }
  const uint64_t hash = hash_mix64(CS_HASH(hash_func, key));

  pthread_rwlock_wrlock(&(h->rwlock));
  int32_t rc =
      GLUE3(phash_, prefix, _put_locked)(h, hash, key, value, h->update);
  pthread_rwlock_unlock(&(h->rwlock));
  return rc;
}

static inline int32_t
GLUE3(phash_, prefix, _atomic_update)(PHTABLE *h, hkey_t key, value_t value,
                                      value_t (*update)(value_t, value_t)) {
  if (h == NULL || update == NULL) {
    return -1;
  }
  uint64_t (*hash_func)(hkey_t) =
      atomic_load_explicit(&h->hash_func, memory_order_acquire);
  if (!CS_HAVE_HASH(hash_func)) {
    return -1;
  }
  const uint64_t hash = hash_mix64(CS_HASH(hash_func, key));

  pthread_rwlock_wrlock(&(h->rwlock));
  int32_t rc = GLUE3(phash_, prefix, _put_locked)(h, hash, key, value, update);
  pthread_rwlock_unlock(&(h->rwlock));
  return rc;
}

/* int32_t phash_prefix_get(phtable_prefix_t *h, hkey_t key, value_t *value)

   Recovers a value from a hash table.  *value can be NULL if the user
   only wants to know if the key is in the table.

   Return Value:
     -1 => an error
     0 => key not found
     1 => key found, *value set to the corresponding value.
*/

static inline int32_t GLUE3(phash_, prefix, _get)(PHTABLE *h, hkey_t key,
                                                  value_t *value) {
  if (h == NULL) {
    return -1;
  }
  uint64_t (*hash_func)(hkey_t) =
      atomic_load_explicit(&h->hash_func, memory_order_acquire);
  if (!CS_HAVE_HASH(hash_func)) {
    return -1;
  }
  uint64_t hash = hash_mix64(CS_HASH(hash_func, key));

  pthread_rwlock_rdlock(&(h->rwlock));
  uint64_t mask = CS_LOAD(h->capacity) - UINT64_C(1);
  uint64_t base = hash & mask;
  uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

  for (uint64_t pos = base;; pos = (pos + step) & mask) {
    if (h->A[pos] == NULL) {
      pthread_rwlock_unlock(&(h->rwlock));
      return 0;
    } else {
      if (h->A[pos] != &(h->deleted) && h->A[pos]->hash == hash) {
        if (CS_KEYEQ(h->comp, key, h->A[pos]->key)) {
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

/* int32_t phash_prefix_remove(phtable_prefix_t *h, hkey_t key, value_t *value)

   Removes key and its value from the table.  *value can be NULL if the user
   does not need the value.

   Return Value:
     -1 => an error
     0 => key not found
     1 => key removed, *value set to the value it had.
*/

static inline int32_t GLUE3(phash_, prefix, _remove)(PHTABLE *h, hkey_t key,
                                                     value_t *value) {
  if (h == NULL) {
    return -1;
  }
  uint64_t (*hash_func)(hkey_t) =
      atomic_load_explicit(&h->hash_func, memory_order_acquire);
  if (!CS_HAVE_HASH(hash_func)) {
    return -1;
  }
  uint64_t hash = hash_mix64(CS_HASH(hash_func, key));

  pthread_rwlock_wrlock(&(h->rwlock));
  uint64_t mask = CS_LOAD(h->capacity) - UINT64_C(1);
  uint64_t base = hash & mask;
  uint64_t step = ((hash >> h->shift) & mask) | UINT64_C(1);

  for (uint64_t pos = base;; pos = (pos + step) & mask) {
    if (h->A[pos] == NULL) {
      pthread_rwlock_unlock(&(h->rwlock));
      return 0;
    } else {
      if (h->A[pos] != &(h->deleted) && h->A[pos]->hash == hash) {
        if (CS_KEYEQ(h->comp, key, h->A[pos]->key)) {
          if (value != NULL) {
            *value = h->A[pos]->value;
          }
          GLUE3(pool_phn_, prefix, _put)
          (h->pool, &h->A[pos]); /* sets it NULL */
          h->A[pos] = &(h->deleted);
          CS_STORE(h->size, CS_LOAD(h->size) - 1);
          pthread_rwlock_unlock(&(h->rwlock));
          return 1;
        }
      }
    }
  }
}

#undef PHPOOL
#undef PHNODE
#undef PHTABLE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting)
   are removed so that they do not leak into the includer */
#undef _unused
#undef LOAD_FACTOR
#undef CS_KEYEQ
#undef CS_HASH
#undef CS_HAVE_HASH
#undef CS_LOAD
#undef CS_STORE
#undef hkey_hash
#undef hkey_equal
