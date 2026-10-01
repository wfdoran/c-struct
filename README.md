# c-struct

Basic data structures with generic types in C11.  Each is a single header which
you include once for every type you need, so there is no library to build and no
`void *` casting for the element type.

## Using a container

Before including a header, define the element type and a label (`prefix`) which
becomes part of every name:

```c
#include <stdio.h>

#define data_t int
#define prefix int
#include <array.h>
#undef prefix
#undef data_t

int main(void) {
  array_int_t *a = array_int_init();
  array_int_append(a, 42);
  printf("%d\n", array_int_get(a, 0));
  array_int_destroy(&a);
  return 0;
}
```

```
gcc -std=gnu11 -I include example.c -o example
```

The headers are `static inline`, so they can be included from any number of `.c`
files, and a type can be instantiated several times with different labels.
`-std=gnu11` is used throughout; the code also needs POSIX (`ssize_t`,
pthreads).  The thread safe hash table needs `-pthread`.

Conventions: functions are named `container_prefix_operation`.  Functions which
can fail return `int32_t` with 0 for success and -1 for an error; lookups return
a struct with a `found` flag, or a count (1 found, 0 not found); a few accessors
use `ASSERT` (`cs_assert.h`) on misuse: it prints `ASSERT failure:`, the file,
function, line and condition to stderr and aborts, and, unlike `assert()`, it is
not removed by `-DNDEBUG`.  The containers never free the data that stored
pointers refer to, unless you give them a free function.

## Containers

* [Array](doc/array.md): growable array with stack, queue, sorting, binary
  search, heap and file operations.
* [Linked List](doc/linked_list.md): doubly linked list with in-place editing
  while walking, and a stable merge sort.
* [Hash Table](doc/hash_table.md): open addressing hash map.
* [Parallel Hash Table](doc/phash_table.md): thread safe hash map, with an
  atomic read-modify-write.
* [Binary Tree](doc/tree.md): balanced (AVL) ordered map with rank queries and
  walks.
* [Priority Queue](doc/pqueue.md): binary heap of key and value pairs.
* [Channels](doc/chan.md): bounded channels, built on atomics rather than locks,
  for passing values between threads, with a non-blocking select.
* [Option](doc/option.md): a value which may be absent.

## Other headers

* `comp.h` and `hash.h`: the default comparison and hash functions the
  containers pick for basic key types (`DEFAULT_COMP`, `DEFAULT_HASH`), and the
  64-bit mixing function used by the hash tables.  Write your own for other
  types.
* `serialize.h`: variable length integer and string encoding used by
  `array_prefix_serialize()`.
* `any.h`: a small tagged union holding one of the basic C types.
* `bin/interval.c`: a stand alone interval arithmetic program (build line at the
  top of the file) which uses the priority queue.

## Tests and demos

```
cd check && make && ./unit_tests     # unit tests for every container
cd demo && make array_demo1          # small example programs, one per feature
```

`future.txt` lists ideas for later.
