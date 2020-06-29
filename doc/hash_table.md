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

### `void hash_prefix_destroy(htable_prefix_t **h_ptr)`

### `int32_t hash_prefix_put(htable_prefix_t *h, key_t key, value_t value)`

### `int32_t hash_prefix_get(const htable_prefix_t *h, key_t key, value_t *value)`

### `int32_t hash_prefix_remove(htable_prefix_t *h, key_t key, value_t *value)`

## Specializing  

### `void hash_prefix_set_hash(htable_prefix_t *h, uint64_t (*hash_func) (key_t))`

### `void hash_prefix_set_comp(htable_prefix_t *h, int (*comp) (key_t, key_t))`

### `void hash_prefix_set_update(htable_prefix_t *h, value_t (*update) (value_t, value_t))`

## Accessors

### `int64_t hash_prefix_get_size(const htable_prefix_t *h)`

### 'int64_t hash_prefix_get_capacity(const htable_prefix_t *h)`

## Interation

### `int32_t hash_prefix_first(const htable_prefix_t *h, hiter_prefix_t **iter_state, key_t *key, value_t *value)`

### `int32_t hash_prefix_next(hiter_prefix_t **iter_state, key_t *key, value_t *value);`

## Global Operations

### `void hash_prefix_apply(htable_prefix_t *h, value_t (*f)(key_t, value_t))`

### `void hash_prefix_apply_r(htable_prefix_t *h, value_t (*f)(key_t, value_t, void*), void *arg)`

## Technical Details