# Array

## Overview

This provides a generic C11 array.  Before including `array.h`, a data type and prefix label must be defined.

```c
#define data_t int
#define prefix int
#incude <array.h>
#undef prefix
#undef data_t
```



## Basic Operations

### `array_prefix_t *array_prefix_init()`

Initializes an empty array. 

### `array_prefix_t *array_prefix_init2(size_t size, data_t default_value)`

Initializes an array with `size` elements set to `default_value`.

### `int32_t array_prefix_destroy(array_prefix_t **a)`

Frees the internal array, frees the `array_prefix_t`, and sets the users pointer to `NULL`.

### `data_t array_prefix_get(const array_prefix_t *a, size_t idx)`

Gets the array element at position `idx`.  If `idx` is out of range and a null value
has been set, the default value is returned; otherwise an assert failure.

### `int32_t arary_prefix_set(array_prefix_t *a, data_t value, size_t idx)`

Sets the entry at position `idx` to `value`.

## Queue

### `void array_prefix_append(array_prefix_t *a, data_t value)`

Appends `value` to the end of the array.  The array will be expanded as needed.

### `data_t array_prefix_pop(array_prefix_t *a)`

Pops the last value off the array.

### `data_t array_prefix_pop_first(array_prefix_t *a)`

Pops the first value off of the array.  Shift the meaning of the indexing: what was in 
position 5 is now in position 4.

## Specialize

### `int32_t array_prefix_set_comp(array_prefix_t *a, int (*comp) (data_t *, data_t *))`

Sets a comparision fucntion for the array data type.  This is not needed for basic datatypes.

### `int32_t array_prefix_set_null_value(array_prefix_t *a, data_t null_value)`

Sets a null value.

## Getters

### `size_t array_prefix_size(const array_prefix_t *a)`

Gives the size of the array.

### `size_t array_prefix_capacity(const array_prefix_t *a)`

Gives the amount of allocated space for the array.

## Collective Operations

### `int32_t array_prefix_sort(array_prefix_t *a)`

Sorts the array.

### `int32_t array_prefix_map(array_prefix_t *a, data_t (*f)(data_t))`

Applies the function `f` to every element in the array.

### `int32_t array_prefix_scan(array_prefix_t *a, data_t (*f)(data_t, data_t))`



### `data_t array_prefix_fold(const array_prefix_t *a, data_t (*f) (data_t, const data_t))`

### `data_t array_prefix_fold2(const array_prefix_t *a, data_t init, data_t (*f) (data_t, const data_t))`

## Copy

### `array_prefix_t *array_prefix_clone(const array_prefix_t *a)`

### `array_prefix_t *array_prefix_deep_clone(const array_prefix_t *a, data_t (*f) (const data_t))`

### `array_prefix_t *array_prefix_slice(const array_prefix_t *a, size_t left, size_t right)`

### `array_prefix_t *array_prefix_deep_slice(const array_prefix_t *a, size_t left, size_t right, data_t (*f) (const data_t))`

## Search

### `size_t array_prefix_index(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect_upper(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect_lower(const array_prefix_t *a, data_t v)`

## Heap

### `int32_t array_prefix_heappush(array_prefix_t *a, data_t value)`

### `data_t array_prefix_heappop(array_prefix_t *a)`

### `int32_t array_prefix_heapify(array_prefix_t *a)`

