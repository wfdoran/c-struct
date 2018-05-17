#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <comp.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define TYPE GLUE3(array_, prefix, _t)

typedef struct {
    data_t *data;
    size_t size;
    size_t capacity;
    int (*comp) (data_t *, data_t *);
} TYPE;

void GLUE3(array_, prefix, _init) (TYPE *a) {
    a->data = malloc(sizeof(data_t));
    assert(a->data != NULL);
    a->size = 0;
    a->capacity = 1;
    data_t temp;
    a->comp = DEFAULT_COMP(temp);
}

void GLUE3(array_, prefix, _set_comp) (TYPE *a, int (*comp) (data_t *, data_t*)) {
    a->comp = comp;
}

void GLUE3(array_, prefix, _sort) (TYPE *a) {
    assert(a->comp != NULL);
    int (*comp) (const void *, const void *) = (int(*)(const void *, const void *)) a->comp;
    qsort(a->data, a->size, sizeof(data_t), comp);
}

data_t GLUE3(array_, prefix, _get) (TYPE *a, size_t idx) {
    assert(idx >= 0 && idx < a->size);
    return a->data[idx] ;
}

void GLUE3(array_, prefix, _set) (TYPE *a, data_t value, size_t idx) {
    assert(idx >= 0 && idx < a->size);
    a->data[idx] = value;
}

void GLUE3(array_, prefix, _append) (TYPE *a, data_t value) {
    if (a->size == a->capacity) {
        data_t *tmp = malloc(2 * a->capacity * sizeof(data_t));
        assert(tmp != NULL);
        memcpy(tmp, a->data, a->capacity * sizeof(data_t));
        free(a->data);
        a->data = tmp;
        a->capacity *= 2;
    }
    a->data[a->size] = value;
    a->size++;
}

size_t GLUE3(array_, prefix, _size) (TYPE *a) {
    return a->size;
}

size_t GLUE3(array_, prefix, _capacity) (TYPE *a) {
    return a->capacity;
}

#undef TYPE
#undef GLUE
#undef GLUE_HELPER
