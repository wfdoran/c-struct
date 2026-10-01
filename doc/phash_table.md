# Parallel Hash Table

A thread safe version of the hash table.  Use it from several threads at once without any locking of your own.
It is set up exactly like `hash_table.h`, and the key and value types are chosen the same way, but the functions
and types start with `phash_` and `phtable_`:

```c
#define hkey_t int
#define value_t int
#define prefix pi
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

phtable_pi_t *h = phash_pi_init(0);
phash_pi_put(h, 7, 70);
```

Programs which use it need `-pthread`.  Every function is `static inline`.  Read `hash_table.md` first; this page only
describes the differences.

## Differences from the plain table

* **One lock.**  The table has a single read-write lock.  `get`, `size` and `capacity` take it for reading, so any number of
  them can run together.  `put`, `remove` and `atomic_update` take it for writing, one at a time.
* **Same results.**  `put`, `get` and `remove` have the same arguments and return values as in `hash_table.md`.  `destroy` and `init` are the same as well.
* **Hash outside the lock.**  The hash function runs before the lock is taken, so a slow hash function does not hold up other threads.
  `phash_prefix_set_hash()` can therefore be called while other threads use the table.  The compare and update functions run while the lock is held, so
  they must not call back into the table.
* **Missing.**  There is no iteration, `clone` or `apply`; they would have to hold the lock for the whole walk.

## Extra function

### `int32_t phash_prefix_atomic_update(phtable_prefix_t *h, hkey_t key, value_t value, value_t (*update) (value_t, value_t))`

Inserts `value` if `key` is not present.  If it is present, its value becomes `update(old_value, value)`.  The whole operation
happens under the write lock, so concurrent updates of the same key cannot lose each other's changes, which is not true of a `get`
followed by a `put`.  For example, counting words from many threads:

```c
int add(int a, int b) { return a + b; }
...
phash_words_atomic_update(h, word, 1, add);
```

Returns 0 on success, and -1 if `h` is `NULL`, `update` is `NULL`, there is no hash function, or memory could not be allocated.
Unlike `put`, it uses the `update` argument rather than the function set with `set_update`.
