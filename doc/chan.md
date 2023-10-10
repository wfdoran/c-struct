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

Allocates and initializes a channel with the given capacity.  To the
user, a `chan_prefix_t` should be viewed as an opaque object.  There
should be no need to directly access its fields

Returns a pointer to the newly allocated chan_prefix_t on success or
NULL on error. 

### `void chan_prefix_destroy(chan_prefix_t **c)`

Frees the internals allocations in a `chan_prefix_t`, frees the
`chan_prefix_t` itself, and sets the user's pointer to it to NULL.
This gives some protection against trying to use the pointer
afterwards.

### `int32_t chan_prefix_send(chan_prefix_t *c, data_t value)`

Puts a value into a channel.  Blocks if the channels is full.

Returns CHAN_SUCCESS on success, CHAN_CLOSED if the channel has been
closed, or CHAN_ERROR on an error.

### `int32_t chan_prefix_recv(chan_prefix_t *c, data_t *value)`

Reads a value from the channel.  Blocks if the channel is empty.

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

Returns CHAN_SUCCESS on successfully reading a value from the channel,
CHAN_EMPTY if the channel is empty (no read), CHAN_CLOSED if the
channel is empyt and closed, or CHAN_ERROR on error.

### `int32_t chan_prefix_close(chan_prefix_t *c)`

Closes a channel.  Future `chan_prefix_send` and `chan_prefix_trysend` will
fail with CHAN_CLOSED.  Future `chan_prefix_recv` and `chan_prefix_tryrecv` will
work until the channel is drained then fail with CHAN_CLOSED.

Returns CHAN_SUCCESS on success of CHAN_CLOSED if the channel was already closed. 
