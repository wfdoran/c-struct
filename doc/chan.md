# Channels

## Overview

Provides a C11 implementation of channels.  The user sets the data
type and prefix label before including `chan.h`.

```c
#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t
```

## Functions

### `chan_prefix_t *chan_prefix_init(int64_t capacity)`

Allocates and initializes a channel with the given capacity.  A capacity below 1
is rounded up to 1; unbuffered (rendezvous) channels are not provided.  To the
user, a `chan_prefix_t` should be viewed as an opaque object.  There should be
no need to directly access its fields

Returns a pointer to the newly allocated chan_prefix_t on success or
NULL on error. 

### `void chan_prefix_destroy(chan_prefix_t **c)`

Frees the internals allocations in a `chan_prefix_t`, frees the
`chan_prefix_t` itself, and sets the user's pointer to it to NULL.
This gives some protection against trying to use the pointer
afterwards.

### `int32_t chan_prefix_send(chan_prefix_t *c, data_t value)`

Puts a value into a channel.  Blocks if the channel is full.

Returns CHAN_SUCCESS on success, CHAN_CLOSED if the channel has been
closed, or CHAN_ERROR on an error.

### `int32_t chan_prefix_recv(chan_prefix_t *c, data_t *value)`

Reads a value from the channel.  Blocks if the channel is empty.

If `value` is `NULL`, the value is read and discarded.

Returns CHAN_SUCCESS on success, CHAN_CLOSED if the channel if
empty and closed, or CHAN_ERROR on an error. 

### `int32_t chan_prefix_trysend(chan_prefix_t *c, data_t value)`

Tries to put a value into the channel.  Does not block if the channel
is full.

Returns CHAN_SUCCESS on successfully putting value into the channel,
CHAN_FULL if the channel is full (value is not put into channel),
CHAN_CLOSED if the channel is closed, or CHAN_ERROR on error.

### `int32_t chan_prefix_tryrecv(chan_prefix_t *c, data_t *value)`

Tries to read a value from a channel.  Does not block if the channel is
empty.

If `value` is `NULL`, the value is read and discarded.

Returns CHAN_SUCCESS on successfully reading a value from the channel,
CHAN_EMPTY if the channel is empty (no read), CHAN_CLOSED if the
channel is empty and closed, or CHAN_ERROR on error.

### `int32_t chan_prefix_close(chan_prefix_t *c)`

Closes a channel.  Future `chan_prefix_send` and `chan_prefix_trysend` will
fail with CHAN_CLOSED.  Future `chan_prefix_recv` and `chan_prefix_tryrecv` will
work until the channel is drained then fail with CHAN_CLOSED.

Returns CHAN_SUCCESS on success or CHAN_CLOSED if the channel was already
closed.

`close` must not be called while a send is in progress.  Close the channel from
the sending side, after the last `chan_prefix_send` or `chan_prefix_trysend` has
returned (when several threads send, after all of them have finished, for
example by joining them).  A send which started before the close but finishes
after a receiver has already seen CHAN_CLOSED can return CHAN_SUCCESS for a
value that no receiver will ever get.  This is the same rule as in Go, where
closing a channel that is still being sent to is a programming error.  Closing a
channel more than once, or from a receiver once the senders are done, is fine.

## Select

`select` waits for the first of several channel operations which can be done
without blocking, like `select` in Go (but without blocking: it returns at once
if nothing is ready, so call it in a loop).  The cases are an array of
`select_prefix_t`:

```c
typedef struct {
  chan_prefix_t *c;      /* the channel */
  int select_type;       /* SELECT_SEND, SELECT_RECV or SELECT_OMIT */
  data_t *value;         /* what to send, or where to put what is received */
} select_prefix_t;
```

```c
select_int_t s[] = {
  {.c = in, .select_type = SELECT_RECV, .value = &x},
  {.c = out, .select_type = SELECT_SEND, .value = &y},
};
for (bool done = false; !done;) {
  switch (select_int_one(2, s)) {
  case 0:            /* a value was received into x */         break;
  case 1:            /* y was sent */                           break;
  case 2:            /* nothing was ready: do something else */ break;
  case SELECT_DONE:  /* every channel is closed */      done = true; break;
  default:           /* CHAN_ERROR */                   done = true; break;
  }
}
```

### `int32_t select_prefix_one(int32_t num_select, select_prefix_t *s)`

Tries the `num_select` cases in random order, so that no ready case can starve
the others, and does the first one which can be done at once.  A send case
needs a `value` to send; a receive case may have `value` set to `NULL`, and the
value is then discarded.  A case with type `SELECT_OMIT` is skipped.

Returns

* the index of the case which was done;
* `num_select` if no case could be done;
* `SELECT_DONE` if there are no cases, or every case is already `SELECT_OMIT`;
* `CHAN_ERROR` if `num_select` is negative or above `SELECT_MAX_CASES` (256),
  `s` is `NULL`, a case has a `NULL` channel, or a send case has a `NULL`
  `value`.  All the cases are checked before any is tried, so this does not
  depend on which one is visited first.

A case whose channel is found closed (a receive from a channel which is closed
and empty, or a send to a closed one) is changed to `SELECT_OMIT` in the array
you passed.  The call which finds the last channel closed returns
`num_select`, and the next call returns `SELECT_DONE`.

### `int32_t select_prefix_option_done(int32_t num_select, select_prefix_t *s, int32_t i)`

Finishes case `i` of the `num_select` cases in `s`: it becomes `SELECT_OMIT`
and its channel is closed.  Returns 0 (also if the channel was already
closed), or `CHAN_ERROR` if `s` is `NULL`, `i` is not in `[0, num_select)`, or
the channel is `NULL`.
