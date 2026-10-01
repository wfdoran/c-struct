# Linked List


### `llist_prefix_t *llist_prefix_init(void)`

### `void llist_prefix_destroy(llist_prefix_t **a_ptr)`

### `int32_t llist_prefix_add_start(llist_prefix_t *a, data_t v)`

### `int32_t llist_prefix_add_end(llist_prefix_t *a, data_t v)`

### `data_t llist_prefix_remove_start(llist_prefix_t *a)`

### `data_t llist_prefix_remove_end(llist_prefix_t *a)`

### `data_t llist_prefix_walk_init_start(llist_prefix_t *a, lnode_prefix_t **n_ptr)`

### `data_t llist_prefix_walk_init_end(llist_prefix_t *a, lnode_prefix_t **n_ptr)`

### `data_t llist_prefix_walk_forward(lnode_prefix_t **n_ptr)`

### `data_t llist_prefix_walk_backwards(lnode_prefix_t **n_ptr)`

### `data_t llist_prefix_remove_forward(llist_prefix_t *a, lnode_prefix_t **n_ptr)`

### `data_t llist_prefix_remove_backwards(llist_prefix_t *a, lnode_prefix_t **n_ptr)`

### `int32_t llist_insert_before(llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t v)`

### `int32_t llist_insert_after(llist_prefix_t *a, lnode_prefix_t **n_ptr, data_t v)`

### `int32_t llist_prefix_msort(llist_prefix_t *a)`

Sorts the list in place with a stable merge sort using the comparison function.  The
comparison function is set with `llist_prefix_set_comp()`; for basic `data_t` such as
`int` or `char*` a default is provided (see `comp.h`).  Runs in O(n log n) time for
every input, uses O(log n) stack, and does not allocate.

Returns 0 on success, or -1 if `a` is `NULL` or no comparison function is available.
