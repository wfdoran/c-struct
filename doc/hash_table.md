# Hash Table

## Overview

Provides a C11 implementation of a hash table, hash map, associative array,
dictionary, whatever name you prefer.  The user provides the type for the keys
and values, along with a prefix label used on the functions.  The following adds
functions for hash table from strings (`char*`) to integers (`int`).

```c
#define hkey_t char*
#define value_t int
#define prefix string
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t
```

The hash table functions and types for this type all contain the prefix value
`string` in their name.  Every function is `static inline`, so the header can be
included from as many `.c` files as you like, and several hash table types can
be used in the same program.  The following code shows off the basic operations.

```c
htable_string_t *h = hash_string_init(0);

hash_string_put(h, "Alice", 20);
hash_string_put(h, "Bob", 25);
hash_string_put(h, "Charlie", 45);

int rc;   // 1 => key present, 0 => key not present, -1 => error
int age;

rc = hash_string_get(h, "Alice", &age);
assert(rc == 1);
rc = hash_string_get(h, "David", &age);
assert(rc == 0);

hash_string_remove(h, "Bob", NULL);

rc = hash_string_get(h, "Bob", &age);
assert(rc == 0);

hash_string_destroy(&h);
```

The table is not safe to use from several threads at once; `phash_table.h` is
the thread safe variant (see `phash_table.md`).

## Basic Operations

In the following descriptions, `prefix` is replaced by whatever name you
provided.

### `htable_prefix_t *hash_prefix_init(int64_t expected_size);`

Allocates and initializes a `htable_prefix_t` which stores the hash entries
and associated information.  Returns `NULL` if memory could not be allocated.
This should be viewed as an opaque structure which the user should never
need to look into and certainly code should not depend on its fields and layout
as these can change between releases.

`expected_size` is the number of entries you expect to store.  The initial size
of the table is the smallest power of 2 which holds that many entries at no more
than 75% occupancy, or 16, whichever is larger, so `hash_prefix_init(0)` is fine
when you have no idea.  The table grows by doubling when it gets too full.

If the key type is one that `hash.h` knows (every integer type including
`long long`, `size_t` and `bool`, `float`, `double`, `char*`, `const char*`), a
hash function is set automatically, and for `char*` and `const char*` keys a
compare function (`strcmp`) as well.  For any other key type, call
`hash_prefix_set_hash()` before using the table.

### `void hash_prefix_destroy(htable_prefix_t **h_ptr)`

Frees the internal hash table, then frees the htable_prefix_t itself, and
sets the pointer itself to `NULL` to avoid using it after calling this routine.
To perform the last step, the user passes the address of the pointer to
the htable_prefix_t.

```c
htable_prefix_t *h = hash_prefix_init(0);
...
hash_prefix_destroy(&h);
assert(h == NULL);
```

This routine does not free the keys or values.  It does not know how.
The routines `hash_prefix_first` and `hash_prefix_next` can be used to
iterate over the hash table and manually free/destroy the keys and values
first.

### `int32_t hash_prefix_put(htable_prefix_t *h, hkey_t key, value_t value)`

Inserts a key/value pair into the hash table.

This performs a shallow copy of the key and value to the hash table.  In
particular, if either hkey_t or value_t is a pointer, the address is stored
in the hash table along with the (current) hash value of the key.
Once inserted, the key should not be altered.

If the key is already in the table, the existing entry is kept and only its
value changes.  If an update function has been set with
`hash_prefix_set_update`, the new value is `update(old_value, value)`; otherwise
`value` replaces the old value.  The hash table does not free the old value, so
use an update function if it needs to be released.  If a compare function is set
(the default for `char*` keys), it decides whether two keys with the same hash
are the same key; without one, equal hashes mean equal keys.

Returns 0 on success (whether the key was new or already present) and -1 on
error (`h` is `NULL`, there is no hash function, or memory could not be
allocated).  A -1 means the entry was not stored.  If memory runs out only while
the table is being enlarged after a successful insert, `put` still returns 0 and
the enlargement is retried by a later `put`; if the table can no longer grow and
one empty slot is left, `put` of a new key returns -1.

### `int32_t hash_prefix_get(const htable_prefix_t *h, hkey_t key, value_t *value)`

Looks up a key.  Returns 1 if the key is present, in which case `*value` is set
to its value, 0 if it is not, and -1 on error.  `value` may be `NULL` if you
only want to know whether the key is present.

### `int32_t hash_prefix_remove(htable_prefix_t *h, hkey_t key, value_t *value)`

Removes a key.  Returns 1 if the key was present and has been removed, in which
case `*value` is set to the value it had (if `value` is not `NULL`), 0 if it was
not present, and -1 on error.  The key and value are not freed.

## Specializing

Set these before putting entries into the table.

### `void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (hkey_t))`

Sets the hash function which maps a key to a `uint64_t`.  `hash.h` provides one
for many basic types.  For more complicated types, write your own.  The table
mixes the bits of whatever it returns, so a simple function is enough.  Equal
keys must give equal hashes.

### `void hash_prefix_set_comp(htable_prefix_t *h, int (*comp) (hkey_t, hkey_t))`

Sets a compare function which is used to tell keys with the same hash apart.  It
should return 0 if the two keys are the same and non-zero if they are different.
If none is set, two keys with the same hash are considered to be the same key.

### `void hash_prefix_set_update(htable_prefix_t *h, value_t (*update) (value_t, value_t))`

Sets a function which combines the old value with the new one when
`hash_prefix_put` finds the key already present.  For example, with `value_t` an
`int`, the update function

```c
int update(int previous, int current) {
  return previous + current;
}
```

makes the table keep a running sum for each key.  Another example, for a `char*`
value which should be freed when replaced:

```c
char* update(char* previous, char* current) {
   free(previous);
   return current;
}
```

## Accessors

### `size_t hash_prefix_size(const htable_prefix_t *h)`

Returns the number of entries in the table.

### `size_t hash_prefix_capacity(const htable_prefix_t *h)`

Returns the number of slots allocated for the table.  When more than 75% of the
slots are in use (by entries or by markers left by removed entries), the table
is rebuilt, doubling in size if it needs the room.

## Iteration

Each entry is visited once, in no particular order.  Do not put or remove
entries during an iteration.

```c
hkey_t key;
value_t value;
hiter_prefix_t *iter;
for (int rc = hash_prefix_first(h, &iter, &key, &value); rc == 0; rc = hash_prefix_next(&iter, &key, &value)) {
    ...
}
```

### `int32_t hash_prefix_first(const htable_prefix_t *h, hiter_prefix_t **iter_state, hkey_t *key, value_t *value)`

Starts an iteration.  Returns 0 and fills in `*key` and `*value` with the first
entry, or returns 1 if the table is empty, or -1 if memory for the iterator
could not be allocated (`*iter_state` is then `NULL`).  Either of `key` and
`value` may be `NULL`.

### `int32_t hash_prefix_next(hiter_prefix_t **iter_state, hkey_t *key, value_t *value)`

Returns 0 and fills in the next entry, or returns 1 when there are no more.
When it returns 1 the iterator has been freed and `*iter_state` is `NULL`.  If
you stop an iteration early, `free()` the iterator yourself.

## Global Operations

### `void hash_prefix_apply(htable_prefix_t *h, value_t (*f)(hkey_t, value_t))`

Replaces the value of every entry by `f(key, value)`.

### `void hash_prefix_apply_r(htable_prefix_t *h, value_t (*f)(hkey_t, value_t, void*), void *arg)`

The same, but `arg` is passed through to `f` as a third argument, for state or
results.

### `htable_prefix_t *hash_prefix_clone(htable_prefix_t *h)`

Makes a copy of the table, including its hash, compare and update functions.
Keys and values are copied by value, so if they are pointers, both tables refer
to the same data.  Returns `NULL` if memory could not be allocated.

## Technical Details

The table uses open addressing: a table is an array of pointers to entries, and
a key's position is found by double hashing, so a lookup follows a probe
sequence which depends on the whole hash.  Each entry is allocated separately.
The hash returned by the hash function is first run through a 64-bit mixing
function, so the quality of the hash function does not matter much; in
particular the integer and floating point hashes in `hash.h` are simple.
Removing an entry leaves a marker, which is reused by later insertions and
dropped when the table is rebuilt.  `hash_prefix_rehash()` rebuilds the table
(it is also done automatically) and returns 0, or -1 if memory could not be
allocated.

`hash.h` hashes `0.0` and `-0.0` alike and gives all NaNs the same hash.  With
no compare function (the default for floating point keys) equal hashes mean
equal keys, so all NaNs are one key.  With a compare function built on `==`, a
NaN key can never be found again, because NaN is not equal to itself, so avoid
NaN keys in that case.
