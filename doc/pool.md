# Memory Pool

A pool hands out space for objects of one type, `data_t`, and takes it back.
`get` and `put` take constant time, the objects are packed together in a few
large blocks instead of being scattered by `malloc`, and everything is freed at
once by `destroy`.  It suits a program which creates and drops many small
objects of one type, such as the nodes of a tree or a hash table (the hash
tables of this library keep their nodes in a pool).

```c
#define data_t int
#define prefix int
#include <pool.h>
#undef prefix
#undef data_t
```

This provides `pool_prefix_t` (the pool) and `pentry_prefix_t` (what is stored
for each object; you do not use it directly).  In the descriptions below,
`prefix` stands for whatever label you chose.  Every function is
`static inline`.

```c
typedef struct {
  double x;
  double y;
  double r;
} circle_t;

#define data_t circle_t
#define prefix circ
#include <pool.h>
#undef prefix
#undef data_t

pool_circ_t *pool = pool_circ_init(100);

circle_t *c = pool_circ_get(pool);
c->x = 1.0;
c->y = 2.0;
c->r = 0.5;

pool_circ_put(pool, &c);        /* the space is given back; c is NULL now */

pool_circ_destroy(&pool);
```

## How it works

The pool is a chain of blocks.  `init` allocates the first block, with room for
`pool_size` objects.  When every object of every block is in use, `get`
allocates another block, **1.5 times as large as the last one** (rounded up, so
a block of 1 is followed by one of 2, 3 by 5), so the pool never runs out as
long as memory lasts.  The factor is `CS_POOL_GROWTH_NUM / CS_POOL_GROWTH_DEN`
(3 / 2); define both before the first include of `pool.h` (or with `-D`) to
change it, as long as the numerator is larger than the denominator.  A smaller
factor leaves less unused space in the last block, a larger one allocates
less often.  Space which is given back goes on a list of
free space, and `get` takes from that list first, the space given back most
recently first.

A block is never moved or freed until the pool is destroyed, so a pointer from
`get` stays valid while the pool grows.

Each object takes the size of a `data_t` plus one pointer (with whatever padding
the compiler adds): 32 bytes for three `double`s, 16 for an `int`.  Choose the
size of the first block with the number of objects you expect, to allocate as
few blocks as possible; a pool which was too small only costs some extra
blocks.

## Basic Operations

### `pool_prefix_t *pool_prefix_init(size_t pool_size)`

Allocates a pool with room for `pool_size` objects.  `init` links all the space
of the block into the list of free space, so it takes time proportional to
`pool_size`.

Returns `NULL` if `pool_size` is 0, if the block would be too large to allocate
(more than `SIZE_MAX / sizeof(pentry_prefix_t)` objects), or if memory could not
be allocated.

### `data_t *pool_prefix_get(pool_prefix_t *pool)`

Returns space for one `data_t`.  The space belongs to you until you give it
back with `put` or destroy the pool.

**The space is not cleared.**  It holds whatever was there, like the result of
`malloc()`: leftovers of an earlier object, or uninitialized memory.  Set every
field you use.

Returns `NULL` if `pool` is `NULL`, or if the pool is full and memory for
another block could not be allocated (the pool is unchanged; space which was
given back can still be taken).

### `int32_t pool_prefix_put(pool_prefix_t *pool, data_t **d_ptr)`

Gives back the space `*d_ptr` points to, so that `get` can hand it out again.
The pointer is passed by address so that `put` can set it to `NULL`: the
variable cannot be used, or given back again, by mistake.

```c
circle_t *c = pool_circ_get(pool);
...
pool_circ_put(pool, &c);        /* c == NULL now */
```

The space must come from this pool, and must not be used after it is given
back.  `put` refuses what it can recognize:

* space which is not in use, because it was already given back (for example
  through a second copy of the pointer);
* space which is in use in a *different* pool.

In both cases it returns -1, changes nothing, and leaves `*d_ptr` as it is.  It
cannot recognize a pointer which does not come from any pool at all (a local
variable, or something from `malloc()`): to find out, it reads the memory next
to the data, which is not yours to read.  Giving back such a pointer is a bug in
the caller.

Returns 0 on success, or -1 if `pool`, `d_ptr` or `*d_ptr` is `NULL`, or the
space is not in use in this pool.

### `int32_t pool_prefix_destroy(pool_prefix_t **p_ptr)`

Frees the pool with all of its blocks, and sets `*p_ptr` to `NULL`.

This frees every block **whether or not the space in it was given back**, so
every pointer from `get` is invalid afterwards.  `destroy` does not look at the
objects: if a `data_t` owns memory (a `char *` you allocated, for example), free
that before you destroy the pool.

Returns 0, or -1 if `p_ptr` or `*p_ptr` is `NULL`, so destroying twice is safe.

## Things to know

* **Not thread safe.**  `get` and `put` change the list of free space without
  any locking, so calls on one pool must not overlap.  Either use one pool per
  thread, lock around the calls yourself, or use the pool of
  [`ppool.md`](ppool.md), which has a mutex built in.  (A container which
  already holds its own lock around every operation does not need either: the
  parallel hash table keeps its nodes in a plain pool and only touches it while
  it holds its write lock.)
* **The space is not cleared**, see `get`.
* **Space must come from this pool**, see `put`.
* **`init(0)` returns `NULL`.**  A pool needs room for at least one object.
  This differs from `array_prefix_init2(0, value)`, where an empty array is
  fine.
* **The pool does not shrink.**  Blocks are only freed by `destroy`, so a pool
  which once held a million objects keeps the memory when they are given back.
  Giving space back makes it available to `get` again, not to the rest of the
  program.
* **Objects are not constructed or destroyed.**  `get` returns space, and `put`
  takes it back; neither runs any code on the data.
