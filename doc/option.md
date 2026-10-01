# Option

A value which may or may not be present, like `Option` in other languages.  It is a small struct which is passed and returned by value, so it needs
no allocation and no destroy function.

```c
#define data_t int
#define prefix int
#include <option.h>
#undef prefix
#undef data_t

option_int_t a = option_int_init(5);
option_int_t b = option_int_init_empty();
```

In the descriptions below, `prefix` stands for whatever label you chose and `option_prefix_t` is the type.  Every function is `static inline`.

An optional sentinel can be given before including the header with `#define sentinel_value some_value`.  An empty option then holds that
value, and `option_prefix_force_get()` returns it for an empty option instead of an ASSERT failure.  The header undefines `sentinel_value` again.

## Creating

### `option_prefix_t option_prefix_init(data_t value)`
### `option_prefix_t option_prefix_init_empty(void)`

An option holding `value`, and an empty option.

### `option_prefix_t option_prefix_clone(option_prefix_t x)`
### `option_prefix_t option_prefix_deep_clone(option_prefix_t x, data_t (*f) (const data_t))`

A copy.  `deep_clone` replaces a held value by `f(value)`, for values which are pointers to data which must be duplicated.  With `f == NULL`
it is the same as `clone`.  An empty option stays empty.

## Reading

### `bool option_prefix_is_set(option_prefix_t x)`

Whether the option holds a value.

### `bool option_prefix_get(option_prefix_t x, data_t *value)`

Returns `true` and stores the value in `*value` if the option holds one, otherwise returns `false` and leaves `*value` alone.  `value` may be `NULL`,
which makes it the same as `is_set`.

### `data_t option_prefix_force_get(option_prefix_t x)`

Returns the held value.  The option must hold one: an ASSERT failure otherwise, unless a sentinel was configured.

### `data_t option_prefix_get_or_else(option_prefix_t x, data_t other)`

Returns the held value, or `other` if the option is empty.

## Changing

### `void option_prefix_set(option_prefix_t *x, data_t value)`

Makes the option hold `value`.

### `bool option_prefix_get_clear(option_prefix_t *x, data_t *value)`

Like `get`, and if there was a value, also leaves the option empty.  Useful to take a value out of an option exactly once.

### `int32_t option_prefix_map(option_prefix_t *x, data_t (*f) (data_t))`

Replaces a held value by `f(value)`; an empty option stays empty.  Returns 0, or -1 if `x` or `f` is `NULL`.
