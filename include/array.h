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


// array_set_data_free
// array_prefix_concat
// array_prefix_merge
// array_prefix_resize
// array_prefix_isheap
// array_prefix_issorted
// array_prefix_filter

typedef struct {
    data_t *data;
    data_t *alloc;
    size_t size;
    size_t capacity;
    int (*comp) (data_t *, data_t *);
    bool have_null_value;
    data_t null_value;
} TYPE;

/* array_prefix_t* array_prefix_init();
   
   Initializes an empty array.
*/
TYPE *GLUE3 (array_, prefix, _init) () {
    TYPE *a = malloc (sizeof (TYPE));
    assert (a != NULL);
    a->alloc = malloc (sizeof (data_t));
    assert (a->alloc != NULL);
    a->data = a->alloc;
    a->size = 0;
    a->capacity = 1;
    data_t temp;
    a->comp = DEFAULT_COMP (temp);
    a->have_null_value = false;
    return a;
}

/* array_prefix_t* array_prefix_init2(size_t size, data_t default_value);

   Initializes an array of size size with the given default value.
*/

TYPE *GLUE3 (array_, prefix, _init2) (size_t size, data_t default_value) {
    TYPE *a = malloc (sizeof (TYPE));
    assert (a != NULL);
    a->alloc = malloc (size * sizeof (data_t));
    assert (a->alloc != NULL);
    a->data = a->alloc;
    a->size = size;
    a->capacity = size;
    data_t temp;
    a->comp = DEFAULT_COMP (temp);
    a->have_null_value = false;
    for (int64_t i = 0; i < size; i++) {
        a->data[i] = default_value;
    }
    return a;
}

/* array_prefix_deep_clone(const array_prefix_t*, data_t(*f)(const data_t)); 

   Makes a deep copy of an array.  The user provided f is uses to
   create/initialize each entry in the clone.
*/
TYPE *GLUE3 (array_, prefix, _deep_clone) (const TYPE *in, data_t (*f) (const data_t)) {
    TYPE *out = malloc (sizeof (TYPE));
    assert (out != NULL);
    out->alloc = malloc (in->size * sizeof (data_t));
    assert (out->alloc != NULL);
    out->data = out->alloc;
    out->size = in->size;
    out->capacity = in->size;
    out->comp = in->comp;
    out->have_null_value = in->have_null_value;
    out->null_value = in->null_value;
    for (int64_t i = 0; i < in->size; i++) {
        out->data[i] = f == NULL ? in->data[i] : f (in->data[i]);
    }
    return out;
}

/* array_prefix_clone(const array_prefix_t *a);

   Makes a copy of an array.    
*/
TYPE *GLUE3 (array_, prefix, _clone) (const TYPE *in) {
    return GLUE3 (array_, prefix, _deep_clone) (in, NULL);
}

/* array_prefix_set_comp(array_prefix_t *a, int (*comp) (data_t*, data_t*)

   Sets a comparision function.  This is required in order to call 
   array_prefix_sort().  
*/

void GLUE3 (array_, prefix, _set_comp) (TYPE *a, int (*comp) (data_t *, data_t *)) {
    a->comp = comp;
}

/* 

   Sets the null value.  This is returned from various getters if the
   array is empty or the request is out of range. 
*/
void GLUE3 (array_, prefix, _set_null_value) (TYPE *a, data_t null_value) {
    a->have_null_value = true;
    a->null_value = null_value;
}

/* 

   Destroys an array.  

*/

void GLUE3 (array_, prefix, _destroy) (TYPE **a_ptr) {
    TYPE *a = *a_ptr;
    free (a->alloc);
    a->alloc = NULL;
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
    free (a);
    a_ptr = NULL;
}

void GLUE3 (array_, prefix, _sort) (TYPE *a) {
    assert (a->comp != NULL);
    int (*comp) (const void *, const void *) = (int (*)(const void *, const void *)) a->comp;
    qsort (a->data, a->size, sizeof (data_t), comp);
}

/* 

  Assumes the array is sorted.  Finds the index rv such that
     a->data[rv] == v
  If such index exists, returns -1.
*/

size_t GLUE3 (array_, prefix, _bisect) (const TYPE *a, data_t v) {
    size_t lo = -1;
    size_t hi = a->size;

    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        int x = a->comp (&a->data[mid], &v);
        if (x == 0) {
            return mid;
        }
        if (x > 0) {
            hi = mid;
        }
        else {
            lo = mid;
        }
    }
    return -1;
}

/* array_prefix_bisect_upper

   Assumes the array is sorted.  Finds the index rv such that
     a->data[rv] <= v
     a->data[rv+1] > v
   Returns -1 if all of the values are greater than v.

   The conventions for array_prefix_bisect_lower and array_bisect_upper
   were selected so that 

   lower_bound = array_prefix_bisect_lower(a, v1);
   upper_bound = array_prefix_bisect_upper(a, v2);
   for (int idx = lower_bound; idx <= upper_bound; idx++) {
       ...
   }
   
   will loop over all indicies with values between v1 and v2 inclusive.
   Also, this handles empty ranges as well.  
*/

size_t GLUE3 (array_, prefix, _bisect_upper) (const TYPE *a, data_t v) {
    size_t lo = -1;
    size_t hi = a->size;

    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        int x = a->comp (&a->data[mid], &v);
        if (x <= 0) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    return lo;
}

/* 
   Assumes the array is sorted.  Finds the index rv such that
     a->data[rv] >= v
     a->data[rv-1] < v
   Returns the size of the array if all of the values are less than v.
*/

size_t GLUE3 (array_, prefix, _bisect_lower) (const TYPE *a, data_t v) {
    size_t lo = -1;
    size_t hi = a->size;

    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        int x = a->comp (&a->data[mid], &v);
        if (x < 0) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    return hi;
}

/* Applies a function to every entry in an array */
void GLUE3 (array_, prefix, _map) (TYPE *a, data_t (*f) (data_t)) {
    for (size_t i = 0; i < a->size; i++) {
        a->data[i] = f (a->data[i]);
    }
}

/* Combines all of the entries in the array using a user provided function.
   Takes the first value as the initial value.  */
data_t GLUE3 (array_, prefix, _fold) (const TYPE *a, data_t (*f) (data_t, const data_t)) {
    data_t rv = a->data[0];
    for (size_t i = 1; i < a->size; i++) {
        rv = f (rv, a->data[i]);
    }
    return rv;
}

/* Combines all of the entries in the array using a user provided function.
   The user provides the initial value. */

data_t GLUE3 (array_, prefix, _fold2) (const TYPE *a, data_t init,
                                       data_t (*f) (data_t, const data_t)) {
    data_t rv = init;
    for (size_t i = 0; i < a->size; i++) {
        rv = f (rv, a->data[i]);
    }
    return rv;
}


void GLUE3 (array_, prefix, _scan) (TYPE *a, data_t (*f) (data_t, data_t)) {
    for (size_t i = 1; i < a->size; i++) {
        a->data[i] = f (a->data[i - 1], a->data[i]);
    }
}

data_t GLUE3 (array_, prefix, _get) (TYPE *a, size_t idx) {
    if (idx < 0 || idx >= a->size) {
        if (a->have_null_value) {
            return a->null_value;
        }
        else {
            assert (idx >= 0 && idx < a->size);
        }
    }
    return a->data[idx];
}

/* 

   Pops the last value off of the array.  Using this and array_prefix_append()
   you get a stack.
*/
data_t GLUE3 (array_, prefix, _pop) (TYPE *a) {
    if (a->size <= 0) {
        if (a->have_null_value) {
            return a->null_value;
        }
        else {
            assert (a->size > 0);
        }
    }
    a->size--;
    return a->data[a->size];
}

/* 

   Pops the first value off of the array.  Using this and array_prefix_append()
   you get a queue.
*/
data_t GLUE3 (array_, prefix, _pop_first) (TYPE *a) {
    if (a->size <= 0) {
        if (a->have_null_value) {
            return a->null_value;
        }
        else {
            assert (a->size > 0);
        }
    }

    data_t rv = a->data[0];
    a->size--;
    a->capacity--;
    a->data += 1;
    return rv;
}

void GLUE3 (array_, prefix, _set) (TYPE *a, data_t value, size_t idx) {
    assert (idx >= 0 && idx < a->size);
    a->data[idx] = value;
}

/* 
   Appends a value to the end of an array. 
*/
void GLUE3 (array_, prefix, _append) (TYPE *a, data_t value) {
    if (a->size == a->capacity) {
        data_t *tmp = malloc (2 * a->capacity * sizeof (data_t));
        assert (tmp != NULL);
        memcpy (tmp, a->data, a->capacity * sizeof (data_t));
        free (a->alloc);
        a->alloc = tmp;
        a->data = a->alloc;
        a->capacity *= 2;
    }
    a->data[a->size] = value;
    a->size++;
}

size_t GLUE3 (array_, prefix, _size) (TYPE *a) {
    return a->size;
}

size_t GLUE3 (array_, prefix, _capacity) (TYPE *a) {
    return a->capacity;
}

#define HEAP_PARENT(x) (((x)-1)/2)
#define HEAP_LEFT_CHILD(x) (2*(x) + 1)
#define HEAP_RIGHT_CHILD(x) (2*(x) + 2)

/* 
   Using array_prefix_heappush() and array_prefix_heappop()
   you get a heap.
*/

void GLUE3 (array_, prefix, _heappush) (TYPE *a, data_t value) {
    assert (a->comp != NULL);
    GLUE3 (array_, prefix, _append) (a, value);
    size_t pos = a->size - 1;
    while (pos != 0) {
        size_t parent = HEAP_PARENT (pos);
        if (a->comp (&(a->data[pos]), &(a->data[parent])) <= 0) {
            break;
        }
        data_t temp = a->data[parent];
        a->data[parent] = a->data[pos];
        a->data[pos] = temp;
        pos = parent;
    }
}

data_t GLUE3 (array_, prefix, _heappop) (TYPE *a) {
    if (a->size <= 0) {
        if (a->have_null_value) {
            return a->null_value;
        }
        else {
            assert (a->size > 0);
        }
    }
    assert (a->comp != NULL);
    data_t rv = a->data[0];
    a->size--;

    if (a->size > 0) {
        a->data[0] = a->data[a->size];
        size_t pos = 0;

        while (true) {
            size_t left = HEAP_LEFT_CHILD (pos);
            size_t right = HEAP_RIGHT_CHILD (pos);

            if (left >= a->size && right >= a->size) {
                break;
            }

            if (right >= a->size) {
                if (a->comp (&a->data[left], &a->data[pos]) > 0) {
                    data_t temp = a->data[pos];
                    a->data[pos] = a->data[left];
                    a->data[left] = temp;
                }
                break;
            }

            if (a->comp (&a->data[pos], &a->data[left]) > 0
                && a->comp (&a->data[pos], &a->data[right]) > 0) {
                break;
            }

            size_t swap = a->comp (&a->data[right], &a->data[left]) > 0 ? right : left;
            data_t temp = a->data[pos];
            a->data[pos] = a->data[swap];
            a->data[swap] = temp;
            pos = swap;
        }

    }
    return rv;
}

void GLUE3 (array_, prefix, _heapify) (TYPE *a) {
    size_t size = a->size;
    a->size = 0;
    for (size_t i = 0; i < size; i++) {
        GLUE3 (array_, prefix, _heappush) (a, a->data[i]);
    }
}

size_t GLUE3 (array_, prefix, _index) (TYPE *a, data_t v) {
    for (size_t i = 0; i < a->size; i++) {
        if (a->comp (&a->data[i], &v) == 0) {
            return i;
        }
    }
    return -1;
}

#undef HEAP_RIGHT_CHILD
#undef HEAP_LEFT_CHILD
#undef HEAP_PARENT

#undef TYPE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
