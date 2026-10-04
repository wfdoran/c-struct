# Array

## Overview

This provides a generic C11 array.  Before including `array.h`, a data type and
prefix label must be defined.

```c
#define data_t int
#define prefix int
#include <array.h>
#undef prefix
#undef data_t
```

The array functions and types all contain the prefix value in their name, so the
code above gives `array_int_t`, `array_int_init()`, `array_int_append()` and so
on.  In the descriptions below, `prefix` stands for whatever label you chose.
Every function is `static inline`, so the header can be included from as many
`.c` files as you like.

An optional null value can be given before including the header with
`#define default_null_value some_value`; the header undefines it again.  See
`array_prefix_set_null_value()`.

## Basic Operations

### `array_prefix_t *array_prefix_init(void)`

Initializes an empty array.  Returns `NULL` if memory could not be allocated.

### `array_prefix_t *array_prefix_init2(size_t size, data_t default_value)`

Initializes an array with `size` elements set to `default_value`.  Returns
`NULL` if memory could not be allocated.

### `int32_t array_prefix_destroy(array_prefix_t **a)`

Frees the internal array, frees the `array_prefix_t`, and sets the users pointer
to `NULL`.  Returns 0, or -1 if there was nothing to destroy.  If the entries
are pointers, the user must free what they point to first; `array_prefix_map()`
can be used for this.

### `data_t array_prefix_get(const array_prefix_t *a, size_t idx)`

Gets the array element at position `idx`.  If `idx` is out of range and a null
value has been set, the null value is returned; otherwise an ASSERT failure.

### `int32_t array_prefix_set(array_prefix_t *a, data_t value, size_t idx)`

Sets the entry at position `idx` to `value`.  Returns 0, or -1 if `a` is `NULL`
or `idx` is out of range.

## Queue and Stack

### `int32_t array_prefix_append(array_prefix_t *a, data_t value)`

Appends `value` to the end of the array.  The array will be expanded as needed.
Returns 0 on success, or -1 if `a` is `NULL` or memory could not be allocated
(the array is unchanged).

### `data_t array_prefix_pop(array_prefix_t *a)`

Pops the last value off the array.  With `array_prefix_append()` this gives a
stack.  If the array is empty, the null value is returned if one has been set;
otherwise an ASSERT failure.

### `data_t array_prefix_pop_first(array_prefix_t *a)`

Pops the first value off of the array.  Shift the meaning of the indexing: what
was in position 5 is now in position 4.  With `array_prefix_append()` this gives
a queue; the slots freed at the front are reused by later appends.  An empty
array behaves as for `array_prefix_pop()`.

## Specialize

### `int32_t array_prefix_set_comp(array_prefix_t *a, int (*comp) (data_t *, data_t *))`

Sets the comparison function for the array data type.  It returns a negative
number, zero, or a positive number as its first argument is less than, equal to,
or greater than its second.  This is not needed for basic datatypes (see
`comp.h`), but is required for sorting, searching and the heap functions with
other types.  Returns 0, or -1 if `a` or `comp` is `NULL`.

### `int32_t array_prefix_set_null_value(array_prefix_t *a, data_t null_value)`

Sets the null value, which `array_prefix_get()`, `array_prefix_pop()`,
`array_prefix_pop_first()` and `array_prefix_heappop()` return instead of an
ASSERT failure when the index is out of range or the array is empty.  Returns 0,
or -1 if `a` is `NULL`.

## Getters

### `size_t array_prefix_size(const array_prefix_t *a)`

Gives the size of the array.

### `size_t array_prefix_capacity(const array_prefix_t *a)`

Gives the number of entries the array can hold before it has to grow.

## Collective Operations

### `int32_t array_prefix_sort(array_prefix_t *a)`

Sorts the array in place with a stable merge sort using the comparison function.
Returns 0 on success, or -1 if `a` is `NULL`, no comparison function is
available, or memory for the temporary buffer could not be allocated.

### `int32_t array_prefix_map(array_prefix_t *a, data_t (*f)(data_t))`

Applies the function `f` to every element in the array, replacing it with the
result.  Returns 0, or -1 if `a` or `f` is `NULL`.

### `int32_t array_prefix_scan(array_prefix_t *a, data_t (*f)(data_t, data_t))`

Performs a scan on the array using `f` to combine values.  `a[0]` remains the
same.  `a[1]` is replaced by `f(a[0],a[1])`.  `a[2]` is replaced by
`f(a[1],a[2])` where `a[1]` is the new value.  And so on down the array.
Returns 0, or -1 if `a` or `f` is `NULL`.

### `data_t array_prefix_fold(const array_prefix_t *a, data_t (*f) (data_t, const data_t))`

Combines all of the entries using `f`.  The first entry is the initial value, so
the array must not be empty (an ASSERT failure otherwise).

### `data_t array_prefix_fold2(const array_prefix_t *a, data_t init, data_t (*f) (data_t, const data_t))`

Combines all of the entries using `f`, starting from the value `init`.

## Copy

### `array_prefix_t *array_prefix_clone(const array_prefix_t *a)`

Makes a copy of an array, including its comparison function and null value.
Returns `NULL` if `a` is `NULL` or memory could not be allocated.

### `array_prefix_t *array_prefix_deep_clone(const array_prefix_t *a, data_t (*f) (const data_t))`

Like `array_prefix_clone()`, but each entry of the copy is created by calling
`f` on the original entry.  Use this when the entries are pointers to data which
must be duplicated.  With `f == NULL` this is `array_prefix_clone()`.

### `array_prefix_t *array_prefix_slice(const array_prefix_t *a, size_t left, size_t right)`

Copies the half-open range `[left, right)` into a new array, so `left == right`
gives an empty array.  Returns `NULL` if `left > right`, `right` is larger than
the size of `a`, or memory could not be allocated.

### `array_prefix_t *array_prefix_deep_slice(const array_prefix_t *a, size_t left, size_t right, data_t (*f) (const data_t))`

Like `array_prefix_slice()`, but each entry is created by calling `f`, as for
`array_prefix_deep_clone()`.

## Search

### `ssize_t array_prefix_index(const array_prefix_t *a, data_t v)`

Linear search.  Returns the index of the first entry which compares equal to
`v`, or -1.  The comparison function must be available.

### `ssize_t array_prefix_bisect(const array_prefix_t *a, data_t v)`

Binary search in an array which is sorted according to the comparison function.
Returns the index of an entry equal to `v` (any one of them if there are
several), or -1.

### `ssize_t array_prefix_bisect_upper(const array_prefix_t *a, data_t v)`

For a sorted array, returns the largest index `i` with `a[i] <= v`, or -1 if
every entry is greater than `v`.

### `ssize_t array_prefix_bisect_lower(const array_prefix_t *a, data_t v)`

For a sorted array, returns the smallest index `i` with `a[i] >= v`, or the size
of the array if every entry is less than `v`.  The two functions are chosen so
that the loop `for (i = bisect_lower(a, v1); i <= bisect_upper(a, v2); i++)`
visits exactly the indices with values between `v1` and `v2` inclusive,
including when that range is empty.

The three `bisect` functions, like `array_prefix_index()`, need a comparison function
and a non-`NULL` array: otherwise an ASSERT failure, since their return values
cannot report an error.

## Heap

The array can be used as a binary heap ordered by the comparison function, with
the largest entry first.  To get the smallest first, set a comparison function
which reverses the order.

### `int32_t array_prefix_heappush(array_prefix_t *a, data_t value)`

Pushes `value` onto the heap.  Returns 0, or -1 if `a` is `NULL`, there is no
comparison function, or memory could not be allocated.

### `data_t array_prefix_heappop(array_prefix_t *a)`

Pops the top entry off the heap.  An empty array behaves as for
`array_prefix_pop()`.

### `int32_t array_prefix_heapify(array_prefix_t *a)`

Rearranges the whole array into a heap in O(n) time.  Returns 0, or -1 if `a` is
`NULL` or there is no comparison function.

## Files

### `int32_t array_prefix_serialize(const array_prefix_t *a, const char *filename)`

Writes the array to a file.  Returns 0, or -1 if an argument is `NULL`, the file
could not be written, or a write failed.  The file holds the entries as raw
bytes of `data_t`, so it can only be read back by a program with the same
`data_t` representation (byte order, size and layout); for pointer types the
values are meaningless.

### `array_prefix_t *array_prefix_deserialize(const char *filename)`

Reads an array written by `array_prefix_serialize()`.  Returns `NULL` if the
file cannot be read, is truncated or damaged, or was written for a different
`prefix` or a `data_t` of a different size.  The comparison function and null
value are not stored; set them again after reading.

## Compiled in comparison

Instead of calling `array_prefix_set_comp()` (a call through a function
pointer), the ordering can be compiled in by defining `data_less` before
including the header:

```c
#define data_t int
#define prefix int
#define data_less(a, b) ((a) < (b))
#include <array.h>
```

`data_less` takes two `data_t` values and must be a strict weak ordering.  Its
arguments are evaluated more than once, so they must have no side effects.  The
compiler can inline it. Sorting 2M `int`s takes 130 ms instead of 190 ms. While
`data_less` is defined, `array_prefix_set_comp()` has no effect, and a
comparison is always available (so a `struct` needs no default).
See `demo/array_demo17.c`.
