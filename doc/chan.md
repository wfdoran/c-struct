# Channels

## Overview

Provides a C11/pthreads implementation of channels.

## Functions

### `chan_prefix_t *chan_prefix_init(int64_t capacity)`

### `void chan_prefix_destroy(chan_prefix_t **c)`

### `int32_t chan_prefix_send(chan_prefix_t *c, data_t value)`

### `int32_t chan_prefix_recv(chan_prefix_t *c, data_t *value)`

### `int32_t chan_prefix_trysend(chan_prefix_t *c, data_t value)`

### `int32_t chan_prefix_tryrecv(chan_prefix_t *c, data_t *value)`

### `int32_t chan_prefix_close(chan_prefix_t *c)`

