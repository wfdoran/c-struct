# Priority Queue

A binary heap which returns the entry with the largest key first.  Each entry is a key of type `data_t` and a `void *` value.
Push and pop take O(log n) time, and peeking at the top takes O(1).

```c
#define data_t int
#define prefix int
#include <pqueue.h>
#undef prefix
#undef data_t
```

This provides `pqueue_prefix_t` and the type returned by `pop` and `peek`:

```c
typedef struct {
	data_t key;
	void *value;
	bool found;
} pqkv_prefix_t;
```

`found` is `false` when the queue was empty.  In the descriptions below, `prefix` stands for whatever label you chose.  Every function
is `static inline`.

By default the largest key comes out first.  To get the smallest first, set a comparison function which reverses the usual order:

```c
static int min_first(int *a, int *b) { return (*a < *b) - (*a > *b); }
...
pqueue_int_set_comp(q, min_first);
```

Keys which compare equal may come out in any order.

### `pqueue_prefix_t *pqueue_prefix_init(void)`

Allocates and returns an empty priority queue, or `NULL` if memory could not be allocated.  For basic key types (see `comp.h`)
a comparison function is set automatically; other types need `pqueue_prefix_set_comp()` before the first `push`.

### `void pqueue_prefix_destroy(pqueue_prefix_t **q_ptr)`

Frees the queue and sets the pointer to `NULL`.  If a value free function has been set, it is applied to the value of every entry still in the queue.  Keys
are never freed.

### `int32_t pqueue_prefix_set_comp(pqueue_prefix_t *q, int (*comp) (data_t *, data_t *))`

Sets the comparison function (negative, zero or positive as the first key is less than, equal to or greater than the second).  Returns 0, or -1 if `q` or `comp` is `NULL`.
Set it while the queue is empty; changing it afterwards breaks the heap order.

### `void pqueue_prefix_set_value_free(pqueue_prefix_t *q, void (*value_free) (void *))`

Sets a function which `pqueue_prefix_destroy()` applies to the value of every remaining entry.  The usual choice is the system `free()` for values allocated with `malloc()`.

### `int32_t pqueue_prefix_push(pqueue_prefix_t *q, data_t key, void *value)`

Adds an entry.  Duplicate keys are allowed.  Returns 0, or -1 if `q` is `NULL`, there is no comparison function, or memory could not be allocated (the queue is unchanged).

### `pqkv_prefix_t pqueue_prefix_pop(pqueue_prefix_t *q)`

Removes and returns the entry with the largest key.  `found` is `false` if the queue is empty.

### `pqkv_prefix_t pqueue_prefix_peek(const pqueue_prefix_t *q)`

Returns the entry with the largest key without removing it.  `found` is `false` if the queue is empty.

### `size_t pqueue_prefix_size(const pqueue_prefix_t *q)`
### `bool pqueue_prefix_is_empty(const pqueue_prefix_t *q)`

The number of entries, and whether there are none.
