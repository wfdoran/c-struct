# Parallel Hash Table

A thread safe version of the hash table.  Use it from several threads at once
without any locking of your own.  It is set up exactly like `hash_table.h`, and
the key and value types are chosen the same way, but the functions and types
start with `phash_` and `phtable_`:

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

Programs which use it need `-pthread`.  Every function is `static inline`.  Read
`hash_table.md` first; this page only describes the differences.

## Differences from the plain table

* **One lock.**  The table has a single read-write lock.  `get` takes it for
  reading, so any number of `get` calls can run together.  `put`, `remove` and
  `atomic_update` take it for writing, one at a time.  `size` and `capacity` do
  not take the lock: they read atomic counters, so they never wait, and while
  other threads change the table they return the value at some moment during
  the call.
* **Same results.**  `put`, `get` and `remove` have the same arguments and
  return values as in `hash_table.md`.  `destroy` and `init` are the same as
  well.
* **Hash outside the lock.**  The hash function runs before the lock is taken,
  so a slow hash function does not hold up other threads.
  `phash_prefix_set_hash()` must be called while the table is empty (it returns
  -1 and changes nothing otherwise, as in `hash_table.md`), and it should be
  called before the table is shared with other threads: the hash of a key is
  computed before the lock is taken, so a `put` which is already running can
  still use the old function.  The compare and update functions run while the
  lock is held, so they must not call back into the table.
* **Pooled nodes.**  The entries come from a memory pool exactly as in
  `hash_table.md` (see "Technical Details" there), so removed entries are
  reused and `destroy` frees everything at once.  The pool is the plain one of
  `pool.md`: it is only touched while the write lock is held, so it does not
  need a lock of its own (`ppool.md` is for programs which share a pool between
  threads directly).
* **Missing.**  There is no iteration, `clone` or `apply`; they would have to
  hold the lock for the whole walk.

## Extra function

### `int32_t phash_prefix_atomic_update(phtable_prefix_t *h, hkey_t key, value_t value, value_t (*update) (value_t, value_t))`

Inserts `value` if `key` is not present.  If it is present, its value becomes
`update(old_value, value)`.  The whole operation happens under the write lock,
so concurrent updates of the same key cannot lose each other's changes, which is
not true of a `get` followed by a `put`.  For example, counting words from many
threads:

```c
int add(int a, int b) { return a + b; }
...
phash_words_atomic_update(h, word, 1, add);
```

Returns 0 if the key was new and was inserted with `value`, 1 if it was already
present and its value became `update(old_value, value)`, and -1 if `h` is
`NULL`, `update` is `NULL`, there is no hash function, or memory could not be
allocated.  Unlike `put`, it uses the
`update` argument rather than the function set with `set_update`.

## Compiled in hash and equality

Instead of `phash_prefix_set_hash()` and `phash_prefix_set_comp()` (calls
through function pointers), the hash and the equality of keys can be compiled
in by defining `hkey_hash` and/or `hkey_equal` before including the header:

```c
#define hkey_t cell_t
#define value_t int
#define prefix cell
#define hkey_hash(k) (((uint64_t) (uint32_t) (k).x << 32) | (uint32_t) (k).y)
#define hkey_equal(a, b) ((a).x == (b).x && (a).y == (b).y)
#include <phash_table.h>
```

`hkey_hash` takes an `hkey_t` and returns a `uint64_t` (it is mixed further
inside the table); `hkey_equal` takes two `hkey_t` and is true for the same key.
Each is optional and independent of the other.  The arguments are evaluated
more than once, so they must have no side effects.  A defined hook replaces the
corresponding function pointer, which is then not used.
