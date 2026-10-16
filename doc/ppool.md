# Thread Safe Memory Pool

A `ppool` is the memory pool of [`pool.md`](pool.md) with one mutex around `get`
and `put`, so that any number of threads can take space from it and give it
back.  Everything else is the same: constant time `get` and `put`, objects
packed in a few large blocks, and everything freed at once by `destroy`.  Read
`pool.md` first; this page only describes what differs.

```c
#define data_t int
#define prefix int
#include <ppool.h>
#undef prefix
#undef data_t
```

This provides `ppool_prefix_t`.  In the descriptions below, `prefix` stands for
whatever label you chose.  Every function is `static inline`.  The header needs
`pthread` (link with `-lpthread`).

```c
typedef struct {
  double x;
  double y;
  double r;
} circle_t;

#define data_t circle_t
#define prefix circ
#include <ppool.h>
#undef prefix
#undef data_t

ppool_circ_t *pool = ppool_circ_init(1000);

/* in any number of threads: */
circle_t *c = ppool_circ_get(pool);
c->x = 1.0;
c->y = 2.0;
c->r = 0.5;
ppool_circ_put(pool, &c);       /* the space is given back; c is NULL now */

/* once the threads are done with the pool: */
ppool_circ_destroy(&pool);
```

## What is safe

* `get` and `put` may be called from any number of threads at once, on the same
  pool.  Each call holds the mutex for its whole duration, so they happen one
  at a time.
* Space may be given back by a different thread than the one which got it.
* A pointer from `get` stays valid until it is given back or the pool is
  destroyed, as in `pool.md`: blocks are never moved.
* Two pools are independent of each other.

## What is not

* **The objects.**  The pool locks its own bookkeeping, not what you store in
  the objects.  Two threads must not use one object at the same time, unless
  you protect it yourself.  After `get`, the object belongs to the caller.
* **`destroy`.**  It does not wait for calls in progress.  Stop or join every
  thread which uses the pool before you destroy it, as you would for any
  `free`.
* **Giving back the same space twice.**  `put` refuses space which is not in
  use, so of two simultaneous calls with the same pointer one succeeds and the
  other returns -1.  But the thread whose call failed holds a pointer to space
  which may already be in use by someone else, so this is a bug in the caller,
  not a feature.  Using space after giving it back is a bug, in a thread as
  anywhere else.

## Basic Operations

### `ppool_prefix_t *ppool_prefix_init(size_t pool_size)`

As `pool_prefix_init()`: a pool with room for `pool_size` objects.  Returns
`NULL` if `pool_size` is 0, if the block would be too large to allocate, or if
memory could not be allocated.

### `data_t *ppool_prefix_get(ppool_prefix_t *pool)`

As `pool_prefix_get()`.  The space is not cleared.  Returns `NULL` if `pool` is
`NULL`, or if the pool is full and memory for another block could not be
allocated (the pool is unchanged; space which was given back can still be
taken).

### `int32_t ppool_prefix_put(ppool_prefix_t *pool, data_t **d_ptr)`

As `pool_prefix_put()`: gives back the space `*d_ptr` points to and sets
`*d_ptr` to `NULL`.  Returns 0 on success, or -1 if `pool`, `d_ptr` or `*d_ptr`
is `NULL`, or the space is not in use in this pool (given back already, or from
another pool); then nothing is changed and `*d_ptr` is left as it is.  As in
`pool.md`, a pointer which comes from no pool at all cannot be recognized.

### `int32_t ppool_prefix_destroy(ppool_prefix_t **p_ptr)`

Frees the pool with all of its blocks and sets `*p_ptr` to `NULL`.  Returns 0,
or -1 if `p_ptr` or `*p_ptr` is `NULL`, so destroying twice is safe.  No other
thread may be using the pool.

## Things to know

* **One mutex.**  With many threads taking and returning space at a high rate,
  they wait for each other.  A call is short (a few pointer writes), so this is
  fine for moderate use, but it does not scale: for heavy use, give each thread
  a pool of its own (`pool.md`, no lock at all), if the objects do not move
  between threads.
* **A container which already has a lock does not need this.**  The hash tables
  keep their nodes in a plain `pool.h` pool, because `put` and `remove` of
  `phash_table.h` already hold the table's write lock whenever they touch it.
* **Both can be used with the same prefix.**  The pool inside a `ppool` has a
  label of its own, so `ppool.h` and `pool.h` instantiated with the same
  `prefix` do not clash.
* **The pool does not shrink**, and space is not cleared, as in `pool.md`.
