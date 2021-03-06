# Hash Table

## Overview

Provides a C11 implemenation of a hash table, hash map, associative array, dictionary, whatever name you prefer.
The user provides the type for the keys and values, along with a prefix label used on the functions.
The following adds functions for hash table from strings (`char*`) to integers (`int`).

```c
#define key_t char*
#define value_t int
#define prefix string
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t
```

The hash table functions and types for this type all contain the prefix value `string` in their name.
The following code shows off the basic operations.

```c
htable_string_t *h = hash_string_init(0);

hash_string_put(h, "Alice", 20);
hash_string_put(h, "Bob", 25);
hash_string_put(h, "Charlie", 45);

int rc;   // 0 => key present, 1 => key not present
int age;

rc = hash_string_get(h, "Alice", &age);
assert(rc == 0);
rc = hash_string_get(h, "David", &age);
assert(rc == 1);

hash_string_remove(h, "Bob", NULL);

rc = hash_string_get(h, "Bob", &age);
assert(rc == 1);

hash_string_destroy(&h);
```

## Basic Operations

The the following descriptions, `prefix` is replaced by whatever name you provided.
This allows you have multiple hash table types in the same program. 

### `htable_prefix_t *hash_prefix_init(int64_t expected_size);`

Allocates and initializes a `htable_prefix_t` which stores the hash entries
and associated information.
This should be viewed as an opaque structures which the user should never
need to look into and certainly code should not depend it fields and layout
as these can change between releases.

The initial size of the table is the first power of 2 larger than or equal to
`expected_size`, or 16 which ever is larger.  The table will dynamically grow
once the occupancy hits 75%, doubling in size.

### `void hash_prefix_destroy(htable_prefix_t **h_ptr)`

Frees the internal hash table, the frees the htable_prefix_t itself, and
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
iterate over the hash table and manually free/destroy the keys and value
first. 

### `int32_t hash_prefix_put(htable_prefix_t *h, key_t key, value_t value)`

Inserts a key/value pair into the hash table.

This performs shallow copy of the key and value to the hash table.  In
particular, if either key_t or value_t is a pointer, the address is stored
in hash table and along with the (current) hash value of the key.
Once inserted, the key should not be altered.  

How does the hash table handle cases where the key already exists?


### `int32_t hash_prefix_get(const htable_prefix_t *h, key_t key, value_t *value)`

### `int32_t hash_prefix_remove(htable_prefix_t *h, key_t key, value_t *value)`

## Specializing  

### `void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (key_t))`

### `void hash_prefix_set_comp(htable_prefix_t *h, int (*comp) (key_t, key_t))`

### `void hash_prefix_set_update(htable_prefix_t *h, value_t (*update) (value_t, value_t))`

## Accessors

### `int64_t hash_prefix_get_size(const htable_prefix_t *h)`

### `int64_t hash_prefix_get_capacity(const htable_prefix_t *h)`

## Interation

### `int32_t hash_prefix_first(const htable_prefix_t *h, hiter_prefix_t **iter_state, key_t *key, value_t *value)`

### `int32_t hash_prefix_next(hiter_prefix_t **iter_state, key_t *key, value_t *value);`

## Global Operations

### `void hash_prefix_apply(htable_prefix_t *h, value_t (*f)(key_t, value_t))`

### `void hash_prefix_apply_r(htable_prefix_t *h, value_t (*f)(key_t, value_t, void*), void *arg)`

## Technical Details