#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

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

// array_prefix_insert 
// array_prefix_remove
// array_prefix_index
// array_prefix_popfirst
// array_prefix_heappush
// array_prefix_heappop
// array_prefix_heapify
// array_prefix_concat
// array_prefix_merge
// array_prefix_resize
// array_prefix_isheap
// array_prefix_issorted

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

data_t GLUE3(array_, prefix, _pop) (TYPE *a) {
    assert(a->size > 0);
    a->size--;
    return a->data[a->size];
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

void GLUE3(array_, prefix, _free) (TYPE *a) {
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
}

void GLUE3(array_, prefix, _copy) (TYPE *src, TYPE *dst) {
    dst = src;
    dst->data = malloc(dst->capacity * sizeof(data_t));
    assert(dst->data != NULL);
    memcpy(dst->data, src->data, src->size * sizeof(data_t));
}    

#define HEAP_PARENT(x) (((x)-1)/2)
#define HEAP_LEFT_CHILD(x) (2*(x) + 1)
#define HEAP_RIGHT_CHILD(x) (2*(x) + 2)

void GLUE3(array_, prefix, _heappush) (TYPE *a, data_t value) {
    assert(a->comp != NULL);
    GLUE3(array_, prefix, _append)(a, value);
    size_t pos = a->size - 1;
    while (pos != 0) {
        size_t parent = HEAP_PARENT(pos);
        if (a->comp(&(a->data[pos]), &(a->data[parent])) <= 0) {
            break;
        }
        data_t temp = a->data[parent];
        a->data[parent] = a->data[pos];
        a->data[pos] = temp;
        pos = parent;
    }        
}

data_t GLUE3(array_, prefix, _heappop) (TYPE *a) {
    assert(a->size > 0);
    assert(a->comp != NULL);
    data_t rv = a->data[0];
    a->size--;
    
    if (a->size > 0) {
        a->data[0] = a->data[a->size];
        size_t pos = 0;
        
        while (true) {
            size_t left = HEAP_LEFT_CHILD(pos);
            size_t right = HEAP_RIGHT_CHILD(pos);
            
            if (left >= a->size && right >= a->size) {
                break;
            }
            
            if (right >= a->size) {
                if (a->comp(&a->data[left], &a->data[pos]) > 0) {
                    data_t temp = a->data[pos];
                    a->data[pos] = a->data[left];
                    a->data[left] = temp;
                }
                break;
            }
            
            if (a->comp(&a->data[pos], &a->data[left]) > 0 && a->comp(&a->data[pos], &a->data[right]) > 0) {
                break;
            }
            
            size_t swap = a->comp(&a->data[right], &a->data[left]) > 0 ? right : left;
            data_t temp = a->data[pos];
            a->data[pos] = a->data[swap];
            a->data[swap] = temp;
            pos = swap;
        }
        
    }
    return rv;
}    

#undef HEAP_RIGHT_CHILD
#undef HEAP_LEFT_CHILD
#undef HEAP_PARENT

#undef TYPE
#undef GLUE
#undef GLUE_HELPER
