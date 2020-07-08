# Array

## Overview

## Basic Operations

### `array_prefix_t *array_prefix_init()`

### `array_prefix_t *array_prefix_init2(size_t size, data_t default_value)`

### `void array_prefix_destroy(array_prefix_t **a)`

### `data_t array_prefix_get(const array_prefix_t *a, size_t idx)`

### `data_t arary_prefix_set(array_prefix_t *a, data_t value, size_t idx)`

## Queue

### `void array_prefix_append(array_prefix_t *a, data_t value)`

### `data_t array_prefix_pop(array_prefix_t *a)`

### `data_t array_prefix_pop_first(array_prefix_t *a)`

## Specialize

### `void array_prefix_set_comp(array_prefix_t *a)`

### `void array_prefix_set_null_value(array_prefix_t *a)`

## Getters

### `size_t array_prefix_size(const array_prefix_t *a)`

### `size_t array_prefix_capacity(const array_prefix_t *a)`

## Collective Operations

### `void array_prefix_sort(array_prefix_t *a)`

### `void array_prefix_map(array_prefix_t *a, data_t (*f)(data_t))`

### `void array_prefix_scan(array_prefix_t *a, data_t (*f)(data_t, data_t))`

### `data_t array_prefix_fold(const array_prefix_t *a, data_t (*f) (data_t, const data_t))`

### `data_t array_prefix_fold2(const array_prefix_t *a, data_t init, data_t (*f) (data_t, const data_t))`

## Copy

### `array_prefix_t *array_prefix_clone(const array_prefix_t *a)`

### `array_prefix_t *array_prefix_deep_clone(const array_prefix_t *a, data_t (*f) (const data_t))`

## Search

### `size_t array_prefix_index(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect_upper(const array_prefix_t *a, data_t v)`

### `size_t array_prefix_bisect_lower(const array_prefix_t *a, data_t v)`

## Heap

### `void array_prefix_heappush(array_prefix_t *a, data_t value)`

### `data_t array_prefix_heappop(array_prefix_t *a)`

### `void array_prefix_heapify(array_prefix_t *a)`

