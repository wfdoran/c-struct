# c-struct

## array

## binary tree

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

### `void tree_prefix_init(tree_prefix_t *a);`

### `void tree_prefix_set_comp(tree_prefix_t *a, int (*comp) (data_t *, data_t *));`

### `void tree_prefix_set_update(tree_prefix_t *a, void *(*update) (void *, void *));`

### `void tree_prefix_insert(tree_prefix_t *a, data_t key, void *value);`

### `void *tree_prefix_delete(tree_prefix_t *a, data_t key);`

### `key_prefix_value_t *tree_prefix_retrieve(tree_prefix_t *a, data_t key);`

### `size_t tree_prefix_size(tree_prefix_t *a);`

### `size_t tree_prefix_height(tree_prefix_t *a);`

### `void tree_prefix_walk_init)(tree_prefix_t *a, void **state);`

### `key_prefix_value_t tree_prefix_walk_next(void **state);`


