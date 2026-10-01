# Binary Tree

```c
#define data_t key_type
#define prefix name
#include <tree.h>
#undef prefix
#undef data_t
```

```c
typedef tree_prefix_t;

typedef struct {
	data_t key;
	void *value;
} key_prefix_value_t;
```

### `tree_prefix_t* tree_prefix_init(void);`

Initializes an empty binary tree.

### `void tree_prefix_destroy(tree_prefix_t **t)`

Destroys a binary tree, frees all of its nodes, and sets the pointer to NULL.

### `int32_t tree_prefix_insert(tree_prefix_t *t, data_t key, void *value);`

Inserts a key-value pair into a binary tree.  Returns 0 on success, or -1 if `t` is `NULL`,
no comparison function has been set, or memory could not be allocated (the tree is unchanged).

### `void* tree_prefix_delete(tree_prefix_t *t, data_t key);`

Removes a node from the tree.  Returns the value for this node.
Returns NULL if not found. 

### `key_prefix_value_t tree_prefix_retrieve(tree_prefix_t *t, data_t key)`

Retrieves but does not delete the key-value pair for a given key. 

### `size_t tree_prefix_size(tree_prefix_t *t);`

Returns the number of nodes in the tree.  This implementation does not
have a fictitious root node.  

### `int tree_prefix_height(tree_prefix_t *t);`

Returns the height of the tree, the number of nodes on the longest path from the root
(0 for an empty tree, 1 for a single node).  The tree is kept balanced (AVL), so for `n`
nodes the height is at least ceil(log2(n + 1)) and at most about 1.44 log2(n + 2).

### `void tree_prefix_walk_init(tree_prefix_t *t, void **state);`

Initializes an in-order walk of the tree.

### `key_prefix_value_t tree_prefix_walk_next(void **state);`

Returns the key-value pair for the current node in walk and steps state.
At the last node, the internals of the state are freed and state is set to NULL. 

### `void tree_prefix_walk_init2(tree_prefix_t *t, data_t key, void **state)`

Initializes the in-order walk at the first node which is equal to or greater than
key.  If there is no such node, `*state` is set to `NULL`.

### `void tree_prefix_set_update(tree_prefix_t *t, void *(*update) (void *, void *));`

The default for `tree_prefix_insert` is to set `value` for the node with
the given value or replace it is this key already exists.  This function
allows you prescribe the behavior. 

### `void tree_prefix_set_value_free(tree_prefix_t *t, void (*value_free)(void *)`

By default, the system `free()` is used to free values.  You can use
this command to set your own free function. 

### `void tree_prefix_set_comp(tree_prefix_t *a, int (*comp) (data_t *f, data_t *));`



### `key_prefix_value_t tree_prefix_rank(tree_prefix_t *t, size_t rank)`

### `size_t tree_prefix_num_less(tree_prefix_t *t, key_t key)`

### `size_t tree_prefix_num_less_equal(tree_prefix_t *t, key_t key)`

### `size_t tree_prefix_num_greater(tree_prefix_t *t, key_t key)`

### `size_t tree_prefix_num_greater_equal(tree_prefix_t *t, key_t key)`



