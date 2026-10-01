# Binary Tree

An ordered map from keys of type `data_t` to `void *` values, kept balanced as
an AVL tree.  Insert, delete and retrieve take O(log n) time, and the tree also
answers rank questions ("how many keys are less than this one", "which key has
rank k").

```c
#define data_t key_type
#define prefix name
#include <tree.h>
#undef prefix
#undef data_t
```

This provides the tree type `tree_prefix_t`, and the type returned by most
operations:

```c
typedef struct {
	data_t key;
	void *value;
	bool found;
} key_prefix_value_t;
```

`found` is `false` when nothing matched, and in that case `key` and `value`
should be ignored.  In the descriptions below, `prefix` stands for whatever
label you chose.  Every function is `static inline`, so the header can be
included from as many `.c` files as you like.

## Setup

### `tree_prefix_t* tree_prefix_init(void);`

Initializes an empty binary tree.  Returns `NULL` if memory could not be
allocated.

### `void tree_prefix_destroy(tree_prefix_t **t)`

Destroys a binary tree, frees all of its nodes, applies the value free function
(if one was set) to every value, and sets the pointer to NULL.

### `void tree_prefix_set_comp(tree_prefix_t *t, int (*comp) (data_t *, data_t *));`

Sets the comparison function for keys.  It returns a negative number, zero, or a
positive number as its first argument is less than, equal to, or greater than
its second.  This is not needed for basic types (see `comp.h`).  Set it before
inserting; changing it for a tree which has entries breaks the ordering.

### `void tree_prefix_set_update(tree_prefix_t *t, void *(*update) (void *, void *));`

The default for `tree_prefix_insert` is to store `value` in the node with the
given key, replacing the old value if the key already exists.  This function
allows you to prescribe the behavior: the node's value becomes
`update(current_value, passed_value)`.  For a new node the current value is
`NULL`.  Any memory management is up to `update`.

### `void tree_prefix_set_value_free(tree_prefix_t *t, void (*value_free)(void *))`

Sets a function which frees a value.  By default no function is set and values
are never freed.  When one is set it is used when `tree_prefix_insert` replaces
the value of an existing key (if no update function is set), and by
`tree_prefix_destroy` for every remaining value.  The most common use is to pass
the system `free()` when the values were allocated with `malloc()`.

## Insert, Delete, Retrieve

### `int32_t tree_prefix_insert(tree_prefix_t *t, data_t key, void *value);`

Inserts a key-value pair into a binary tree.  Returns 0 on success, or -1 if `t`
is `NULL`, no comparison function has been set, or memory could not be allocated
(the tree is unchanged).

### `key_prefix_value_t tree_prefix_delete(tree_prefix_t *t, data_t key);`

Removes the node with the given key from the tree and returns its key and value;
the value is not freed.  `found` is `false` if there is no such node.  The key
returned is the one stored in the tree, which matters when the keys are pointers
to allocated memory.

### `key_prefix_value_t tree_prefix_delete_min(tree_prefix_t *t)`
### `key_prefix_value_t tree_prefix_delete_max(tree_prefix_t *t)`

Remove the node with the smallest or largest key and return its key and value.
`found` is `false` if the tree is empty.  These do not use the comparison
function.

### `key_prefix_value_t tree_prefix_retrieve(const tree_prefix_t *t, data_t key)`

Retrieves but does not delete the key-value pair for a given key.

### `key_prefix_value_t tree_prefix_retrieve_min(const tree_prefix_t *t)`
### `key_prefix_value_t tree_prefix_retrieve_max(const tree_prefix_t *t)`

Retrieve, without deleting, the key and value of the smallest or largest key.
`found` is `false` if the tree is empty.

## Size and Rank

### `size_t tree_prefix_size(const tree_prefix_t *t);`

Returns the number of nodes in the tree.  This implementation does not
have a fictitious root node.

### `int tree_prefix_height(const tree_prefix_t *t);`

Returns the height of the tree, the number of nodes on the longest path from the
root (0 for an empty tree, 1 for a single node).  The tree is kept balanced
(AVL), so for `n` nodes the height is at least ceil(log2(n + 1)) and at most
about 1.44 log2(n + 2).

### `key_prefix_value_t tree_prefix_get_rank(const tree_prefix_t *t, size_t rank)`

Returns the key and value with the given rank, counting from 0 in key order:
rank 0 is the smallest key.  `found` is `false` if `rank` is not less than the
size of the tree.

### `size_t tree_prefix_num_less(const tree_prefix_t *t, data_t key)`
### `size_t tree_prefix_num_less_equal(const tree_prefix_t *t, data_t key)`
### `size_t tree_prefix_num_greater(const tree_prefix_t *t, data_t key)`
### `size_t tree_prefix_num_greater_equal(const tree_prefix_t *t, data_t key)`

Return the number of nodes whose key is less than, less than or equal to,
greater than, or greater than or equal to `key`.  The key itself does not need
to be in the tree.

## Walking the Tree

A walk uses a `void *` state variable.  Initialize it, then step it with the
matching `next` function while it is not `NULL`.

```c
void *state;
for (tree_prefix_walk_init(t, &state); state != NULL; ) {
    key_prefix_value_t kv = tree_prefix_walk_next(&state);
    ...
}
```

### `void tree_prefix_walk_init(const tree_prefix_t *t, void **state);`

Initializes an in-order walk of the tree, starting at the smallest key.
`*state` is `NULL` if the tree is empty.

### `void tree_prefix_walk_init2(const tree_prefix_t *t, data_t key, void **state)`

Initializes the in-order walk at the first node which is equal to or greater
than key.  If there is no such node, `*state` is set to `NULL`.

### `key_prefix_value_t tree_prefix_walk_next(void **state);`

Returns the key-value pair for the current node in the walk and steps `state` to
the next node.  At the last node, `state` is set to `NULL`.  Nothing is
allocated, so there is nothing to free if you stop early.  Do not call it when
`state` is `NULL`.  The tree must not be modified during a walk.

### `void tree_prefix_postwalk_init(const tree_prefix_t *t, void **state)`
### `key_prefix_value_t tree_prefix_postwalk_next(void **state)`

The same, but a post-order walk: every node comes after its children, ending
with the root.  This is the order to use to free or destroy things node by node.

### `void tree_prefix_print(const tree_prefix_t *t, void (*node_print) (data_t, void *))`

Prints a picture of the tree sideways to standard output, one node per line,
calling `node_print(key, value)` to print each node's contents.  Useful for
debugging.
