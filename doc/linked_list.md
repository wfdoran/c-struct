# Linked List

A generic doubly linked list.  Before including `linked_list.h`, a data type, a
prefix label and a null value must be defined.

```c
#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef prefix
#undef data_t
```

The functions and types all contain the prefix in their name: `llist_prefix_t`
is the list and `lnode_prefix_t` is a position in it.  In the descriptions
below, `prefix` stands for whatever label you chose.  Every function is
`static inline`, so the header can be included from as many `.c` files as you
like.

`null_value` is returned by the functions that remove and return a value when
there is nothing to remove: removing from an empty list, or with a `NULL` node
variable.  It cannot be told apart from a stored value equal to `null_value`,
so use `size` to check.  The walking functions do not use it (see below).  The
header undefines `null_value` again after it has used it.

The list does not own what the entries refer to: if `data_t` is a pointer, the
user frees the pointed-to data.

## Basic Operations

### `llist_prefix_t *llist_prefix_init(void)`

Initializes an empty list.  Returns `NULL` if memory could not be allocated.

### `void llist_prefix_destroy(llist_prefix_t **a_ptr)`

Frees all of the nodes and the list itself, and sets the user's pointer to
`NULL`.  It does not free what the entries point to.

### `void llist_prefix_set_comp(llist_prefix_t *a, int (*comp) (data_t *, data_t *))`

Sets the comparison function used by `llist_prefix_msort()`.  For basic types
(`int`, `double`, `char*`, ...) one is provided by `comp.h`; other types need
one set here.

### `size_t llist_prefix_size(const llist_prefix_t *a)`

Returns the number of entries in the list.

### `int32_t llist_prefix_add_start(llist_prefix_t *a, data_t v)`
### `int32_t llist_prefix_add_end(llist_prefix_t *a, data_t v)`

Add a new entry at the start or end of the list.  Return 0, or -1 if `a` is
`NULL` or memory could not be allocated.

### `data_t llist_prefix_remove_start(llist_prefix_t *a)`
### `data_t llist_prefix_remove_end(llist_prefix_t *a)`

Remove the first or last entry and return its value, or `null_value` if the list
is empty.

## Walking the List

A walk is controlled by a node variable, `lnode_prefix_t *`, which is an
ordinary variable: nothing is allocated.  The functions return 0 when the node
variable is at an entry, in which case `*value` (which may be `NULL`) holds its
value, and 1 when there is no entry, in which case the node variable is `NULL`.
That happens at the end of the list, and for an empty list.  Once the node
variable is `NULL`, `next` and `prev` keep returning 1.

```c
lnode_int_t *n;
int x;
for (int32_t rc = llist_int_first(a, &n, &x); rc == 0; rc = llist_int_next(&n, &x)) {
    ...
}
```

### `int32_t llist_prefix_first(const llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t *value)`
### `int32_t llist_prefix_last(const llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t *value)`

Put `*n_ptr` at the first or last node.

### `int32_t llist_prefix_next(lnode_prefix_t **n_ptr, data_t *value)`
### `int32_t llist_prefix_prev(lnode_prefix_t **n_ptr, data_t *value)`

Move `*n_ptr` to the next or previous node.

## Changing the List While Walking

### `data_t llist_prefix_remove_forward(llist_prefix_t *a, lnode_prefix_t **n_ptr)`
### `data_t llist_prefix_remove_backwards(llist_prefix_t *a, lnode_prefix_t **n_ptr)`

Remove the current node, return its value, and move `*n_ptr` to the next
(`remove_forward`) or previous (`remove_backwards`) node, which is `NULL` at the
end.  Return `null_value` if `*n_ptr` is `NULL`.

### `int32_t llist_prefix_insert_before(llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t v)`
### `int32_t llist_prefix_insert_after(llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t v)`

Insert a new entry before or after the current node, and move `*n_ptr` to the
new node.  Return 0, or -1 if `*n_ptr` is `NULL` (use `add_start` or `add_end`
to put the first entry into an empty list) or memory could not be allocated.

## Sorting

### `int32_t llist_prefix_msort(llist_prefix_t *a)`

Sorts the list in place with a stable merge sort using the comparison function.
The comparison function is set with `llist_prefix_set_comp()`; for basic
`data_t` such as `int` or `char*` a default is provided (see `comp.h`).  Runs in
O(n log n) time for every input, uses O(log n) stack, and does not allocate.

Returns 0 on success, or -1 if `a` is `NULL` or no comparison function is
available.

### `void llist_prefix_add_node(llist_prefix_t *a, lnode_prefix_t *n)`

Appends an already allocated node to the end of the list without allocating.
The node must not be in any list.  This is mainly a building block for code
which rearranges nodes itself.
