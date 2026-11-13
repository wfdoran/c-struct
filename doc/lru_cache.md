# LRU Cache

## Overview

Provides a C11 implementation of a least recently used (LRU) cache: a map from
keys to values which holds at most a fixed number of entries.  When it is full,
putting a new key removes the entry which was used least recently, where both
`put` and `get` count as a use.  The typical use is to remember the results of
an expensive computation:

```c
double v;
if (lru_sqrt_get(lru, n, &v) != 1) {
  v = expensive_sqrt(n);
  lru_sqrt_put(lru, n, v);
}
```

The user provides the type for the keys and values, along with a prefix label
used on the functions.  The following adds functions for a cache from integers
(`int`) to doubles (`double`).

```c
#define hkey_t int
#define value_t double
#define prefix sqrt
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t
```

The cache functions and types for this type all contain the prefix value `sqrt`
in their name.  Every function is `static inline`, so the header can be included
from as many `.c` files as you like, and several cache types can be used in the
same program.  The following code shows off the basic operations.

```c
lru_sqrt_t *lru = lru_sqrt_init(2); // room for 2 entries

lru_sqrt_put(lru, 4, 2.0);
lru_sqrt_put(lru, 9, 3.0);

int rc;   // 1 => key present, 0 => key not present, -1 => error
double v;

rc = lru_sqrt_get(lru, 4, &v);  // 4 is now the most recently used
assert(rc == 1 && v == 2.0);

lru_sqrt_put(lru, 16, 4.0);     // full: removes 9, the least recently used

rc = lru_sqrt_get(lru, 9, &v);
assert(rc == 0);

lru_sqrt_destroy(&lru);
```

The cache is not safe to use from several threads at once.  Note that this
includes `get`, which changes the order of the entries.  The one exception is
`peek`, which changes nothing: several threads may `peek` at the same time, as
long as no other function runs on the cache meanwhile.

## Basic Operations

In the following descriptions, `prefix` is replaced by whatever name you
provided.

### `lru_prefix_t *lru_prefix_init(int64_t capacity)`

Allocates and initializes a `lru_prefix_t` which holds at most `capacity`
key/value pairs.  This should be viewed as an opaque structure: code should not
depend on its fields and layout, as these can change between releases.

All of the memory the cache will ever use is allocated here, so `put` and `get`
never allocate.  Returns `NULL` if `capacity <= 0`, if `capacity` is absurdly
large, or if memory could not be allocated.

If the key type is one that `hash.h` knows (every integer type including
`long long`, `size_t` and `bool`, `float`, `double`, `char*`, `const char*`), a
hash function is set automatically, and for `char*` and `const char*` keys a
compare function (`strcmp`) as well.  For any other key type, call
`lru_prefix_set_hash()` before using the cache.

### `void lru_prefix_destroy(lru_prefix_t **lru_ptr)`

Calls the evict function (if one was set, see `lru_prefix_set_evict`) on every
entry still in the cache, from the most to the least recently used.  Then frees
the cache and sets the pointer to `NULL` to avoid using it after calling this
routine.  To perform the last step, the user passes the address of the pointer
to the `lru_prefix_t`.

```c
lru_prefix_t *lru = lru_prefix_init(100);
...
lru_prefix_destroy(&lru);
assert(lru == NULL);
```

Without an evict function, the keys and values are not freed.

### `int32_t lru_prefix_clear(lru_prefix_t *lru)`

Removes every entry from the cache.  The evict function (if set) is called on
each, from the most to the least recently used, just as `destroy` does.  The
cache keeps its capacity and the functions which were set, and can be used
again; the memory is kept too, as nothing was allocated per entry.  Since the
cache is empty afterwards, `lru_prefix_set_hash` can be called again.

Returns 0, or -1 if `lru` is `NULL`.

### `int32_t lru_prefix_put(lru_prefix_t *lru, hkey_t key, value_t value)`

Inserts a key/value pair as the most recently used entry.  This is a shallow
copy: if `hkey_t` or `value_t` is a pointer, the address is stored.  Once
inserted, the key should not be altered.

If the key is new and the cache is full, the least recently used entry is
removed first, and the evict function (if set) is called on its key and value.

If the key is already in the cache, the existing entry is kept, becomes the
most recently used, and only its value changes.  If an update function has been
set with `lru_prefix_set_update`, the new value is `update(old_value, value)`;
otherwise `value` replaces the old value.  The evict function is **not** called
on the old value, so use an update function if it needs to be released.  The
stored key is also kept: the `key` passed to this call is not stored, and still
belongs to the caller.

Returns 0 if the key was new and has been inserted, 1 if the key was already
present and its value was replaced (or combined by the update function), and -1
on error (`lru` is `NULL`, or there is no hash function).  A -1 means nothing
was changed.  **A return of 1 is not an error**, so test for failure with
`rc < 0`, not `rc != 0`.

### `int32_t lru_prefix_get(lru_prefix_t *lru, hkey_t key, value_t *value)`

Looks up a key.  If it is present, it becomes the most recently used entry and
`*value` is set to its value.  `value` may be `NULL` if you only want to know
whether the key is present; that still counts as a use.

If a clone function has been set with `lru_prefix_set_clone`, `*value` is
`clone`'s copy of the stored value.  Otherwise it is a plain copy of the stored
`value_t`.  For a pointer type this means the caller only borrows the value: a
later `put` may evict the entry and the evict function may free it (see
[Ownership](#ownership-of-keys-and-values)).

Returns 1 if the key is present, 0 if it is not, and -1 on error (`lru` is
`NULL`, there is no hash function, or the clone function failed).  If the clone
function fails, the cache is unchanged.

### `int32_t lru_prefix_peek(const lru_prefix_t *lru, hkey_t key, value_t *value)`

Looks up a key like `lru_prefix_get`, but does not count as a use: the order of
the entries is not changed, so peeking at an entry does not save it from being
evicted.  This is the lookup to use when checking on the cache (for statistics
or debugging) should not affect what it keeps.

`*value` is set the same way as by `get`, with the clone function if one was
set, and `value` may be `NULL`.  The return values are the same as for `get`.

## Accessors

### `size_t lru_prefix_size(const lru_prefix_t *lru)`

Returns the number of entries in the cache, or 0 if `lru` is `NULL`.  Once the
cache has filled up it stays full, apart from `clear`.

### `size_t lru_prefix_capacity(const lru_prefix_t *lru)`

Returns the most entries the cache can hold, the `capacity` given to `init`, or
0 if `lru` is `NULL`.

## Specializing

### `int32_t lru_prefix_set_hash(lru_prefix_t *lru, uint64_t (*hash_func) (hkey_t))`

Sets the hash function which maps a key to a `uint64_t`.  `hash.h` provides one
for many basic types.  For more complicated types, write your own.  The cache
mixes the bits of whatever it returns, so a simple function is enough.  Equal
keys must give equal hashes.

It must be called while the cache is empty, since the entries already stored
could not be found with a different function.  With entries in the cache it is
refused and nothing is changed.

Returns 0, or -1 if `lru` is `NULL` or the cache has entries.

### `int32_t lru_prefix_set_comp(lru_prefix_t *lru, int (*comp) (hkey_t, hkey_t))`

Sets a compare function which is used to tell keys with the same hash apart.  It
should return 0 if the two keys are the same and non-zero if they are different.
If none is set, two keys with the same hash are considered to be the same key.
Returns 0, or -1 if `lru` is `NULL`.

### `int32_t lru_prefix_set_update(lru_prefix_t *lru, value_t (*update) (value_t, value_t))`

Sets a function which combines the old value with the new one when
`lru_prefix_put` finds the key already present.  The stored value becomes
`update(old_value, value)`.  For a `char*` value owned by the cache, it frees
the value being replaced:

```c
char* update(char* previous, char* current) {
  if (previous != current) {
    free(previous);
  }
  return current;
}
```

The check matters: putting the pointer already stored for a key calls
`update(p, p)`, and an unconditional `free` would leave the cache holding a
dangling pointer.  Returns 0, or -1 if `lru` is `NULL`.

### `int32_t lru_prefix_set_clone(lru_prefix_t *lru, int (*clone) (value_t, value_t *))`

Sets a function used by `lru_prefix_get` to give the caller a value of its own.
`clone(stored, &out)` sets `out` and returns 0 on success, or non-zero on
failure.  It can make a deep copy, or, for reference counted values, take
another reference (see below).  Returns 0, or -1 if `lru` is `NULL`.

### `int32_t lru_prefix_set_evict(lru_prefix_t *lru, void (*evict) (hkey_t, value_t))`

Sets a function which is called with the key and value of every entry which
leaves the cache: the least recently used entry when `put` needs room for a new
key, and every entry left at `destroy`.  It is not called when `put` replaces
the value of an existing key; that is the job of the update function.  Returns
0, or -1 if `lru` is `NULL`.

## Ownership of keys and values

For plain values such as `int` or `double` none of this matters, and no
functions need to be set.  When the keys or values are pointers to memory which
the cache should own, the functions above work together:

| When                                  | What happens to the memory                      |
|---------------------------------------|-------------------------------------------------|
| `put` of a new key                    | the cache takes the key and the value           |
| `put` of a key already present        | the cache takes the value; the old value goes to `update`; the key passed in stays with the caller |
| `get` or `peek` with a clone function    | the caller owns what `clone` gives it        |
| `get` or `peek` without a clone function | the caller borrows the value until the next `put`, `clear` or `destroy` |
| an entry is removed to make room         | its key and value go to `evict`              |
| `clear` or `destroy`                     | every key and value left goes to `evict`     |

For example, a cache from `int` to strings which it owns frees them in
`evict`, copies them in `clone`, and frees the replaced one in `update`:

```c
#include <stdlib.h>
#include <string.h>

#define hkey_t int
#define value_t char *
#define prefix words
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t

static void evict_words(int key, char *value) {
  (void) key; // nothing to free for an int
  free(value);
}

static int clone_words(char *stored, char **out) {
  *out = strdup(stored);
  return *out == NULL ? -1 : 0;
}

static char *update_words(char *previous, char *current) {
  if (previous != current) {
    free(previous);
  }
  return current;
}

int main(void) {
  lru_words_t *lru = lru_words_init(100);
  lru_words_set_evict(lru, evict_words);
  lru_words_set_clone(lru, clone_words);
  lru_words_set_update(lru, update_words);

  lru_words_put(lru, 7, strdup("seven")); // the cache owns the string

  char *mine;
  if (lru_words_get(lru, 7, &mine) == 1) {
    /* mine is a copy: it stays valid even if 7 is evicted */
    free(mine);
  }

  lru_words_destroy(&lru); // frees "seven" through evict_words
  return 0;
}
```

When the values are large, copying each one in `get` can cost more than the
cache saves.  With reference counted values, `clone` takes a new reference
instead of copying, and `evict` and `update` drop the cache's reference.  A
value then lives as long as either the cache or a caller holds it, and every
`get` of the same key returns the same object:

```c
static int clone_primes(primes *stored, primes **out) {
  stored->refs++;
  *out = stored;
  return 0;
}

static void evict_primes(int key, primes *value) {
  primes_unref(value); // frees it if the cache held the last reference
}
```

`put` then hands one reference to the cache, so a caller who also keeps the
value takes a second reference first.

The demos show each case: `demo/lru_demo1.c` (plain values),
`demo/lru_demo2.c` (owned strings with `clone`, `evict` and `update`) and
`demo/lru_demo3.c` (reference counted values).

## Technical Details

The cache is a hash table with separate chaining combined with a doubly linked
list of the entries in the order they were used.  Each entry is a node which
lives in both: `get` finds the node through the hash table and moves it to the
front of the list, and when the cache is full `put` takes the node at the back
of the list, unlinks it from both, and reuses it for the new key.  `put`,
`get` and `peek` take constant time on average; `clear` and `destroy` visit
every entry.

All of the nodes are allocated at once by `init`, as one array of `capacity`
nodes, along with the hash table.  Nothing is allocated or freed after that
until `destroy`, so a full cache does not touch `malloc` at all.  The number of
buckets is the smallest power of 2 which is at least `capacity`, so the chains
hold at most one node on average.  Each node stores, besides the key and the
value, the key's hash and four pointers.

The hash returned by the hash function is first run through a 64-bit mixing
function, so the quality of the hash function does not matter much; in
particular the integer and floating point hashes in `hash.h` are simple.  The
stored hash is compared before the keys, so the compare function is only called
when the hashes match.

`clear` calls `evict` on each entry and then empties the hash table, which
takes time in proportion to the capacity.  There is not yet a way to remove a
single entry.

## Compiled in hash and equality

Instead of `lru_prefix_set_hash()` and `lru_prefix_set_comp()` (calls through
function pointers), the hash and the equality of keys can be compiled in by
defining `hkey_hash` and/or `hkey_equal` before including the header, as for
the hash tables:

```c
#define hkey_t cell_t
#define value_t int
#define prefix cell
#define hkey_hash(k) (((uint64_t) (uint32_t) (k).x << 32) | (uint32_t) (k).y)
#define hkey_equal(a, b) ((a).x == (b).x && (a).y == (b).y)
#include <lru_cache.h>
```

`hkey_hash` takes an `hkey_t` and returns a `uint64_t` (it is mixed further
inside the cache); `hkey_equal` takes two `hkey_t` and is true for the same key.
Each is optional and independent of the other.  The arguments are evaluated
more than once, so they must have no side effects.  A defined hook replaces the
corresponding function pointer, which is then not used.  The macros are removed
at the end of the header.
