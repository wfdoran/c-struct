#include <stdlib.h>
#include <stdint.h>
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

#define LRU_CACHE GLUE3(lru_, prefix, _t)
#define LRU_NODE GLUE3(lru_node_, prefix, _t)

typedef struct LRU_NODE {
  uint64_t hash;
  hkey_t key;
  value_t value;

  /* Each node is part of two double linked lists:
       next, prev : linked list of used nodes in history order
       hnext, hprev : linked list of used nodes with same hash
  */
  struct LRU_NODE *next;
  struct LRU_NODE *prev;
  struct LRU_NODE *hnext;
  struct LRU_NODE *hprev;
} LRU_NODE;

typedef struct LRU_CACHE {
  struct LRU_NODE *head; // most recently used
  struct LRU_NODE *tail; // least recently used, the next to be evicted

  struct LRU_NODE **htable;
  int log2_hash_size;
  uint64_t (*hash_func)(hkey_t);
  int (*comp)(hkey_t, hkey_t);

  void (*evict)(hkey_t, value_t); // optional method to clean up
                                  // the key and value fields in
                                  // LRU_NODE when it is cleared
                                  // from the cache

  value_t (*update)(value_t, value_t); // optional method to update
                                       // the value in a LRU_NODE
                                       // during a put when that
                                       // key exists.

  int (*clone)(value_t, value_t *); // optional: if value_t is a
                                    // pointer type, this method
                                    // is used to make a clone of
                                    // the value in the LRU.
                                    // The caller owns it.

  int64_t num_used;   // nodes of alloc in use: alloc[0 .. num_used-1]
  int64_t alloc_size; // the capacity of the cache
  struct LRU_NODE *alloc;
} LRU_CACHE;

/* lru_prefix_t *lru_prefix_init(int64_t capacity);

   Allocates a cache which holds at most capacity key/value pairs.
   All of the nodes are allocated here; put and get never allocate.

   If hash.h and comp.h recognize hkey_t, the hash and comp functions
   are set.  Otherwise the user must call lru_prefix_set_hash before
   the first put.

   Return Value:
    NULL => an error (capacity <= 0, or out of memory)
    otherwise => a new LRU
*/
static inline LRU_CACHE *GLUE3(lru_, prefix, _init)(int64_t capacity) {
  if (capacity <= 0 || (uint64_t) capacity > SIZE_MAX / sizeof(LRU_NODE) ||
      capacity > (INT64_C(1) << 60)) {
    return NULL;
  }
  LRU_CACHE *lru = malloc(sizeof(LRU_CACHE));
  if (lru == NULL) {
    return NULL;
  }

  /* the number of buckets is the smallest power of 2 >= capacity, so
     the chains average at most one node when the cache is full */
  lru->log2_hash_size = 0;
  while ((INT64_C(1) << lru->log2_hash_size) < capacity) {
    lru->log2_hash_size++;
  }
  lru->htable = calloc((size_t) 1 << lru->log2_hash_size, sizeof(LRU_NODE *));
  lru->alloc = malloc((size_t) capacity * sizeof(LRU_NODE));
  if (lru->htable == NULL || lru->alloc == NULL) {
    free(lru->htable);
    free(lru->alloc);
    free(lru);
    return NULL;
  }
  lru->num_used = 0;
  lru->alloc_size = capacity;
  lru->head = NULL;
  lru->tail = NULL;

  /* If possible set a default hash and comp function for the key type. */
  hkey_t temp;
  lru->hash_func = DEFAULT_HASH(temp);
  lru->comp = DEFAULT_COMP_TYPE(temp);
  CS_UNUSED(temp);
  lru->evict = NULL;
  lru->update = NULL;
  lru->clone = NULL;

  return lru;
}

/* int32_t lru_prefix_set_hash(lru_prefix_t *lru,
                               uint64_t (*hash_func)(hkey_t));

   Sets the hash function on keys.  It must be called while the cache
   is empty, since the entries already stored could not be found
   afterwards.

   Return Value:
    0 => success
    -1 => error (lru is NULL, or the cache has entries; nothing is changed)
*/
static inline int32_t
GLUE3(lru_, prefix, _set_hash)(LRU_CACHE *lru, uint64_t (*hash_func)(hkey_t)) {
  if (lru == NULL || lru->num_used > 0) {
    return -1;
  }
  lru->hash_func = hash_func;
  return 0;
}

/* int32_t lru_prefix_set_comp(lru_prefix_t *lru, int (*comp)(hkey_t, hkey_t));

   Sets the function which decides if two keys with the same hash are
   the same key: it returns 0 if they are equal, non-zero otherwise.
   Without one, keys with equal hash values are taken to be the same
   key.

   Return Value:
    0 => success
    -1 => error (lru is NULL; nothing is changed)
*/
static inline int32_t
GLUE3(lru_, prefix, _set_comp)(LRU_CACHE *lru, int (*comp)(hkey_t, hkey_t)) {
  if (lru == NULL) {
    return -1;
  }
  lru->comp = comp;
  return 0;
}

/* int32_t lru_prefix_set_update(lru_prefix_t *lru,
                                 value_t (*update)(value_t, value_t));

   When a put finds its key already in the cache, the default is to
   overwrite the stored value with the new one.  An update function
   instead combines them: the stored value becomes update(stored,
   new).

   If the cache owns its values (they are freed by evict), the default
   overwrite loses the old value.  Set an update which frees it,
   checking for the same pointer being put again:

     char *update(char *previous, char *current) {
       if (previous != current) {
         free(previous);
       }
       return current;
     }

   Return Value:
    0 => success
    -1 => error (lru is NULL; nothing is changed)
*/
static inline int32_t GLUE3(lru_, prefix,
                            _set_update)(LRU_CACHE *lru,
                                         value_t (*update)(value_t, value_t)) {
  if (lru == NULL) {
    return -1;
  }
  lru->update = update;
  return 0;
}

/* int32_t lru_prefix_set_clone(lru_prefix_t *lru,
                                int (*clone)(value_t, value_t *));

   By default get copies the stored value_t out.  If value_t is a
   pointer owned by the cache, that copy is only borrowed: a later put
   may evict the entry and evict may free it.  A clone function gives
   the caller a value of its own instead.  clone(stored, &out) returns
   0 on success and non-zero on failure.  Besides a deep copy, it can
   increment a reference count which evict decrements.

   Return Value:
    0 => success
    -1 => error (lru is NULL; nothing is changed)
*/
static inline int32_t GLUE3(lru_, prefix,
                            _set_clone)(LRU_CACHE *lru,
                                        int (*clone)(value_t, value_t *)) {
  if (lru == NULL) {
    return -1;
  }
  lru->clone = clone;
  return 0;
}

/* int32_t lru_prefix_set_evict(lru_prefix_t *lru,
                                void (*evict)(hkey_t, value_t));

   Sets a function called with the key and value of each entry which
   leaves the cache: the least recently used entry when a put needs
   room, and every entry at destroy.  It is not called when put
   replaces a value; see lru_prefix_set_update for that.

   Return Value:
    0 => success
    -1 => error (lru is NULL; nothing is changed)
*/
static inline int32_t GLUE3(lru_, prefix,
                            _set_evict)(LRU_CACHE *lru,
                                        void (*evict)(hkey_t, value_t)) {
  if (lru == NULL) {
    return -1;
  }
  lru->evict = evict;
  return 0;
}

/* void lru_prefix_destroy(lru_prefix_t **lru_ptr);

   Runs evict (if set) on every entry in the cache, from most to least
   recently used, then frees the cache and sets the caller's pointer
   to NULL.
*/
static inline void GLUE3(lru_, prefix, _destroy)(LRU_CACHE **lru_ptr) {
  if (lru_ptr == NULL || *lru_ptr == NULL) {
    return;
  }
  LRU_CACHE *lru = *lru_ptr;
  if (lru->evict != NULL) {
    for (LRU_NODE *n = lru->head; n != NULL; n = n->next) {
      lru->evict(n->key, n->value);
    }
  }
  free(lru->htable);
  free(lru->alloc);
  free(lru);
  *lru_ptr = NULL;
}

/* LRU_NODE *lru_prefix_find(const lru_prefix_t *lru, hkey_t key,
                             uint64_t hash);

   The node holding key, or NULL.
*/
CS_PRIVATE static inline LRU_NODE *
GLUE3(lru_, prefix, _find)(const LRU_CACHE *lru, hkey_t key, uint64_t hash) {
  const uint64_t mask = (UINT64_C(1) << lru->log2_hash_size) - 1;
  for (LRU_NODE *n = lru->htable[hash & mask]; n != NULL; n = n->hnext) {
    if (n->hash == hash && CS_KEYEQ(lru->comp, key, n->key)) {
      return n;
    }
  }
  return NULL;
}

/* void lru_prefix_move_to_front(lru_prefix_t *lru, LRU_NODE *n);

   Makes n, already in the history list, the most recently used.
*/
CS_PRIVATE static inline void
GLUE3(lru_, prefix, _move_to_front)(LRU_CACHE *lru, LRU_NODE *n) {
  if (n == lru->head) {
    return;
  }
  /* n is not the head, so it has a prev */
  n->prev->next = n->next;
  if (n->next != NULL) {
    n->next->prev = n->prev;
  } else {
    lru->tail = n->prev;
  }
  n->prev = NULL;
  n->next = lru->head;
  lru->head->prev = n;
  lru->head = n;
}

/* int32_t lru_prefix_put(lru_prefix_t *lru, hkey_t key, value_t value);

   Inserts a key/value pair as the most recently used entry.  If the
   cache is full, the least recently used entry is removed first and
   evict (if set) is called on it.

   If the key is already in the cache, its value is replaced by value,
   or by update(stored, value) if an update function was set, and the
   entry becomes the most recently used.  The stored key is kept: the
   key passed in is not stored, and still belongs to the caller.

   Return Value:
    1 => ok, the key was already in the cache and its value was replaced
         (or combined with the new value by the update function)
    0 => ok, the key was new and has been inserted
    -1 => error (lru is NULL or has no hash function; nothing is changed)
*/
static inline int32_t GLUE3(lru_, prefix, _put)(LRU_CACHE *lru, hkey_t key,
                                                value_t value) {
  if (lru == NULL || !CS_HAVE_HASH(lru->hash_func)) {
    return -1;
  }
  const uint64_t hash = hash_mix64(CS_HASH(lru->hash_func, key));
  const uint64_t mask = (UINT64_C(1) << lru->log2_hash_size) - 1;

  LRU_NODE *n = GLUE3(lru_, prefix, _find)(lru, key, hash);
  if (n != NULL) {
    n->value = lru->update == NULL ? value : lru->update(n->value, value);
    GLUE3(lru_, prefix, _move_to_front)(lru, n);
    return 1;
  }

  if (lru->num_used < lru->alloc_size) {
    n = &lru->alloc[lru->num_used++];
  } else {
    /* full: take the least recently used node out of both lists and reuse it */
    n = lru->tail;
    lru->tail = n->prev;
    if (lru->tail != NULL) {
      lru->tail->next = NULL;
    } else {
      lru->head = NULL;
    }
    if (n->hprev != NULL) {
      n->hprev->hnext = n->hnext;
    } else {
      lru->htable[n->hash & mask] = n->hnext;
    }
    if (n->hnext != NULL) {
      n->hnext->hprev = n->hprev;
    }
    if (lru->evict != NULL) {
      lru->evict(n->key, n->value);
    }
  }

  n->hash = hash;
  n->key = key;
  n->value = value;

  LRU_NODE **bucket = &lru->htable[hash & mask];
  n->hprev = NULL;
  n->hnext = *bucket;
  if (*bucket != NULL) {
    (*bucket)->hprev = n;
  }
  *bucket = n;

  n->prev = NULL;
  n->next = lru->head;
  if (lru->head != NULL) {
    lru->head->prev = n;
  } else {
    lru->tail = n;
  }
  lru->head = n;
  return 0;
}

/* int32_t lru_prefix_get(lru_prefix_t *lru, hkey_t key, value_t *value);

   Looks up key and, if found, makes it the most recently used entry.  value
   can be NULL if the user only wants to know if the key is in the cache.

   *value is set with clone if one was set, otherwise it is a plain copy of
   the stored value (see lru_prefix_set_clone).  If clone fails, the cache is
   unchanged.

   Since get changes the order of the entries, it is not safe to call at the
   same time as any other function on the same cache, including get.

   Return Value:
    1 => key in table, *value set
    0 => key not in table
    -1 => an error (lru is NULL, no hash function, or clone failed)
*/
static inline int32_t GLUE3(lru_, prefix, _get)(LRU_CACHE *lru, hkey_t key,
                                                value_t *value) {
  if (lru == NULL || !CS_HAVE_HASH(lru->hash_func)) {
    return -1;
  }
  const uint64_t hash = hash_mix64(CS_HASH(lru->hash_func, key));
  LRU_NODE *n = GLUE3(lru_, prefix, _find)(lru, key, hash);
  if (n == NULL) {
    return 0;
  }
  if (value != NULL) {
    if (lru->clone != NULL) {
      if (lru->clone(n->value, value) != 0) {
        return -1;
      }
    } else {
      *value = n->value;
    }
  }
  GLUE3(lru_, prefix, _move_to_front)(lru, n);
  return 1;
}

#undef LRU_NODE
#undef LRU_CACHE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header are removed so that they do not
   leak into the includer */
#undef CS_KEYEQ
#undef CS_HASH
#undef CS_HAVE_HASH
#undef hkey_hash
#undef hkey_equal
