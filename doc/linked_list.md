# Linked List


### `llist_prefix_t *llist_prefix_init()`

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

