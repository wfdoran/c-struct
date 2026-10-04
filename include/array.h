#include <stdlib.h>
#include "cs_assert.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include <comp.h>

#include "serialize.h"

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define TYPE GLUE3(array_, prefix, _t)

#define _unused(x) ((void) (x))

// create new array
// array_prefix_filter

// update current array
// array_prefix_concat
// array_prefix_resize

typedef struct {
  data_t *data;
  data_t *alloc;
  size_t size;
  size_t capacity;
  int (*comp)(data_t *, data_t *);
  bool have_null_value;
  data_t null_value;
} TYPE;

/* array_prefix_t* array_prefix_init(void);

   Initializes an empty array.
*/
static inline TYPE *GLUE3(array_, prefix, _init)(void) {
  TYPE *a = malloc(sizeof(TYPE));
  if (a == NULL) {
    return NULL;
  }
  a->alloc = malloc(sizeof(data_t));
  if (a->alloc == NULL) {
    free(a);
    return NULL;
  }
  a->data = a->alloc;
  a->size = 0;
  a->capacity = 1;
  data_t temp;
  a->comp = DEFAULT_COMP(temp);
  _unused(temp);
  _unused(DEFAULT_COMP_TYPE(temp));

#ifdef default_null_value
  a->null_value = default_null_value;
  a->have_null_value = true;
#else
  a->have_null_value = false;
#endif
  return a;
}

/* array_prefix_t* array_prefix_init2(size_t size, data_t default_value);

   Initializes an array of size size with the given default value.
*/

static inline TYPE *GLUE3(array_, prefix, _init2)(size_t size,
                                                  data_t default_value) {
  if (size > SIZE_MAX / sizeof(data_t)) {
    return NULL;
  }
  TYPE *a = malloc(sizeof(TYPE));
  if (a == NULL) {
    return NULL;
  }
  /* never malloc(0): NULL would be indistinguishable from failure */
  a->alloc = malloc((size > 0 ? size : 1) * sizeof(data_t));
  if (a->alloc == NULL) {
    free(a);
    return NULL;
  }
  a->data = a->alloc;
  a->size = size;
  a->capacity = size;
  data_t temp;
  a->comp = DEFAULT_COMP(temp);
  _unused(temp);
#ifdef default_null_value
  a->null_value = default_null_value;
  a->have_null_value = true;
#else
  a->have_null_value = false;
#endif
  for (size_t i = 0; i < size; i++) {
    a->data[i] = default_value;
  }
  return a;
}

/* array_prefix_deep_clone(const array_prefix_t*, data_t(*f)(const data_t));

   Makes a deep copy of an array.  The user provided f is used to
   create/initialize each entry in the clone.
*/
static inline TYPE *
GLUE3(array_, prefix, _deep_clone)(const TYPE *in, data_t (*f)(const data_t)) {
  if (in == NULL) {
    return NULL;
  }
  TYPE *out = malloc(sizeof(TYPE));
  if (out == NULL) {
    return NULL;
  }
  out->alloc = malloc((in->size > 0 ? in->size : 1) * sizeof(data_t));
  if (out->alloc == NULL) {
    free(out);
    return NULL;
  }
  out->data = out->alloc;
  out->size = in->size;
  out->capacity = in->size;
  out->comp = in->comp;
  out->have_null_value = in->have_null_value;
  if (out->have_null_value) {
    out->null_value = in->null_value;
  }
  if (f != NULL) {
    for (size_t i = 0; i < in->size; i++) {
      out->data[i] = f(in->data[i]);
    }
  } else {
    memcpy(out->data, in->data, in->size * sizeof(data_t));
  }
  return out;
}

/* array_prefix_clone(const array_prefix_t *a);

   Makes a copy of an array.
*/
static inline TYPE *GLUE3(array_, prefix, _clone)(const TYPE *in) {
  return GLUE3(array_, prefix, _deep_clone)(in, NULL);
}

/* array_prefix_deep_slice(const array_prefix_t *in, size_t left, size_t right,
                           data_t (*f)(const data_t));

   Makes a new array from the half-open range [left, right) of in, so
   left == right gives an empty array.  Returns NULL if in is NULL,
   left > right, right > size, or memory is exhausted.  The user
   provided f, if not NULL, is used to create each entry.
*/
static inline TYPE *
GLUE3(array_, prefix, _deep_slice)(const TYPE *in, size_t left, size_t right,
                                   data_t (*f)(const data_t)) {
  if (in == NULL) {
    return NULL;
  }
  if (right < left || right > in->size) {
    return NULL;
  }
  const size_t size = right - left;
  TYPE *out = malloc(sizeof(TYPE));
  if (out == NULL) {
    return NULL;
  }
  out->alloc = malloc((size > 0 ? size : 1) * sizeof(data_t));
  if (out->alloc == NULL) {
    free(out);
    return NULL;
  }
  out->data = out->alloc;
  out->size = size;
  out->capacity = size;
  out->comp = in->comp;
  out->have_null_value = in->have_null_value;
  if (out->have_null_value) {
    out->null_value = in->null_value;
  }
  if (f != NULL) {
    for (size_t i = 0; i < size; i++) {
      out->data[i] = f(in->data[i + left]);
    }
  } else {
    memcpy(out->data, &in->data[left], size * sizeof(data_t));
  }
  return out;
}

static inline TYPE *GLUE3(array_, prefix, _slice)(const TYPE *in, size_t left,
                                                  size_t right) {
  return GLUE3(array_, prefix, _deep_slice)(in, left, right, NULL);
}

/* array_prefix_set_comp(array_prefix_t *a, int (*comp) (data_t*, data_t*)

   Sets a comparision function.  This is required in order to call
   array_prefix_sort().
*/

static inline int32_t
GLUE3(array_, prefix, _set_comp)(TYPE *a, int (*comp)(data_t *, data_t *)) {
  if (a == NULL || comp == NULL) {
    return -1;
  }
  a->comp = comp;
  return 0;
}

/*

   Sets the null value.  This is returned from various getters if the
   array is empty or the request is out of range.
*/
static inline int32_t GLUE3(array_, prefix,
                            _set_null_value)(TYPE *a, data_t null_value) {
  if (a == NULL) {
    return -1;
  }
  a->have_null_value = true;
  a->null_value = null_value;
  return 0;
}

/*

   Destroys an array.  If the array entries contains pointers, the user
   needs to destroy these first manually.  array_prefix_map() could be
   used to do this.
*/

static inline int32_t GLUE3(array_, prefix, _destroy)(TYPE **a_ptr) {
  if (a_ptr == NULL) {
    return -1;
  }
  TYPE *a = *a_ptr;
  if (a == NULL) {
    return -1;
  }
  free(a->alloc);
  a->alloc = NULL;
  a->data = NULL;
  a->size = 0;
  a->capacity = 0;
  free(a);
  *a_ptr = NULL;
  return 0;
}

/*
    Sorts the array.
*/

/* Stable merge sort of the n entries at v, using comp directly (qsort would
   need the comparison function cast to an incompatible type).  tmp must have
   room for n / 2 entries. */
static inline void GLUE3(array_, prefix,
                         _sort_range)(int (*comp)(data_t *, data_t *),
                                      data_t *v, data_t *tmp, size_t n) {
  if (n <= 16) {
    for (size_t i = 1; i < n; i++) {
      data_t x = v[i];
      size_t j = i;
      while (j > 0 && comp(&x, &v[j - 1]) < 0) {
        v[j] = v[j - 1];
        j--;
      }
      v[j] = x;
    }
    return;
  }

  size_t half = n / 2;
  GLUE3(array_, prefix, _sort_range)(comp, v, tmp, half);
  GLUE3(array_, prefix, _sort_range)(comp, v + half, tmp, n - half);
  if (comp(&v[half - 1], &v[half]) <= 0) {
    return;
  }

  /* merge: the left half moves to tmp, and the output never overtakes the right
   * half */
  memcpy(tmp, v, half * sizeof(data_t));
  size_t i = 0;
  size_t j = half;
  size_t k = 0;
  while (i < half && j < n) {
    if (comp(&v[j], &tmp[i]) < 0) {
      v[k++] = v[j++];
    } else {
      v[k++] = tmp[i++];
    }
  }
  while (i < half) {
    v[k++] = tmp[i++];
  }
}

/* Sorts the array (a stable sort, O(n log n) comparisons) using the comparison
   function.  Returns -1 if a is NULL, there is no comparison function, or
   memory for the temporary buffer could not be allocated, and 0 otherwise. */
static inline int32_t GLUE3(array_, prefix, _sort)(TYPE *a) {
  if (a == NULL) {
    return -1;
  }
  if (a->comp == NULL) {
    return -1;
  }
  if (a->size < 2) {
    return 0;
  }

  data_t *tmp = malloc((a->size / 2 + 1) * sizeof(data_t));
  if (tmp == NULL) {
    return -1;
  }
  GLUE3(array_, prefix, _sort_range)(a->comp, a->data, tmp, a->size);
  free(tmp);
  return 0;
}

/*

  Assumes the array is sorted.  Finds the index rv such that
     a->data[rv] == v
  If no such index exists, returns -1.
*/

static inline ssize_t GLUE3(array_, prefix, _bisect)(const TYPE *a, data_t v) {
  ASSERT(a != NULL);
  ASSERT(a->comp != NULL);
  ssize_t lo = -1;
  ssize_t hi = (ssize_t) a->size;

  while (hi - lo > 1) {
    ssize_t mid = lo + (hi - lo) / 2;
    int x = a->comp(&a->data[mid], &v);
    if (x == 0) {
      return mid;
    }
    if (x > 0) {
      hi = mid;
    } else {
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

static inline ssize_t GLUE3(array_, prefix, _bisect_upper)(const TYPE *a,
                                                           data_t v) {
  ASSERT(a != NULL);
  ASSERT(a->comp != NULL);
  ssize_t lo = -1;
  ssize_t hi = (ssize_t) a->size;

  while (hi - lo > 1) {
    ssize_t mid = lo + (hi - lo) / 2;
    int x = a->comp(&a->data[mid], &v);
    if (x <= 0) {
      lo = mid;
    } else {
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

static inline ssize_t GLUE3(array_, prefix, _bisect_lower)(const TYPE *a,
                                                           data_t v) {
  ASSERT(a != NULL);
  ASSERT(a->comp != NULL);
  ssize_t lo = -1;
  ssize_t hi = (ssize_t) a->size;

  while (hi - lo > 1) {
    ssize_t mid = lo + (hi - lo) / 2;
    int x = a->comp(&a->data[mid], &v);
    if (x < 0) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return hi;
}

/* Applies a function to every entry in an array */
static inline int32_t GLUE3(array_, prefix, _map)(TYPE *a,
                                                  data_t (*f)(data_t)) {
  if (a == NULL || f == NULL) {
    return -1;
  }
  for (size_t i = 0; i < a->size; i++) {
    a->data[i] = f(a->data[i]);
  }
  return 0;
}

/* Combines all of the entries in the array using a user provided function.
   Takes the first value as the initial value.  */
static inline data_t
GLUE3(array_, prefix, _fold)(const TYPE *a, data_t (*f)(data_t, const data_t)) {
  ASSERT(a != NULL);
  ASSERT(f != NULL);
  ASSERT(a->size > 0);
  data_t rv = a->data[0];
  for (size_t i = 1; i < a->size; i++) {
    rv = f(rv, a->data[i]);
  }
  return rv;
}

/* Combines all of the entries in the array using a user provided function.
   The user provides the initial value. */

static inline data_t GLUE3(array_, prefix,
                           _fold2)(const TYPE *a, data_t init,
                                   data_t (*f)(data_t, const data_t)) {
  ASSERT(a != NULL);
  ASSERT(f != NULL);
  data_t rv = init;
  for (size_t i = 0; i < a->size; i++) {
    rv = f(rv, a->data[i]);
  }
  return rv;
}

static inline int32_t GLUE3(array_, prefix,
                            _scan)(TYPE *a, data_t (*f)(data_t, data_t)) {
  if (a == NULL || f == NULL) {
    return -1;
  }
  for (size_t i = 1; i < a->size; i++) {
    a->data[i] = f(a->data[i - 1], a->data[i]);
  }
  return 0;
}

/*

   Gets the value at a particular index.  If a null value has been provided
   and the index is out of range, the null value is returned.  Otherwise,
   index out of range results in an ASSERT failure.
*/
static inline data_t GLUE3(array_, prefix, _get)(const TYPE *a, size_t idx) {
  ASSERT(a != NULL);
  if (idx >= a->size) {
    if (a->have_null_value) {
      return a->null_value;
    } else {
      ASSERT(idx < a->size);
    }
  }
  return a->data[idx];
}

/*
   Pops the last value off of the array.  Using this and array_prefix_append()
   you get a stack.
*/
static inline data_t GLUE3(array_, prefix, _pop)(TYPE *a) {
  ASSERT(a != NULL);
  if (a->size <= 0) {
    if (a->have_null_value) {
      return a->null_value;
    } else {
      ASSERT(a->size > 0);
    }
  }
  a->size--;
  return a->data[a->size];
}

/*
   Pops the first value off of the array.  Using this and array_prefix_append()
   you get a queue.
*/
static inline data_t GLUE3(array_, prefix, _pop_first)(TYPE *a) {
  ASSERT(a != NULL);
  if (a->size <= 0) {
    if (a->have_null_value) {
      return a->null_value;
    } else {
      ASSERT(a->size > 0);
    }
  }

  data_t rv = a->data[0];
  a->size--;
  a->capacity--;
  a->data += 1;
  return rv;
}

static inline int32_t GLUE3(array_, prefix, _set)(TYPE *a, data_t value,
                                                  size_t idx) {
  if (a == NULL) {
    return -1;
  }
  if (idx >= a->size) {
    return -1;
  }
  a->data[idx] = value;
  return 0;
}

/*
   Appends a value to the end of an array.

   return value:
     -1 => error (a is NULL or out of memory; the array is unchanged)
      0 => ok
*/
static inline int32_t GLUE3(array_, prefix, _append)(TYPE *a, data_t value) {
  if (a == NULL) {
    return -1;
  }
  if (a->size == a->capacity) {
    /* capacity counts the slots from data to the end of the block; the
       front slots in front of data were freed by array_prefix_pop_first() */
    size_t front = a->data - a->alloc;

    if (front > 0 && front >= a->size) {
      /* enough room was freed at the front: slide the entries down */
      memmove(a->alloc, a->data, a->size * sizeof(data_t));
      a->data = a->alloc;
      a->capacity += front;
    } else {
      if (a->capacity > SIZE_MAX / 2 / sizeof(data_t)) {
        return -1;
      }
      size_t new_capacity = a->capacity == 0 ? 1 : 2 * a->capacity;
      if (new_capacity > SIZE_MAX / sizeof(data_t) - front) {
        return -1;
      }
      data_t *tmp = realloc(a->alloc, (front + new_capacity) * sizeof(data_t));
      if (tmp == NULL) {
        return -1;
      }
      a->alloc = tmp;
      a->data = tmp + front;
      a->capacity = new_capacity;
    }
  }
  a->data[a->size] = value;
  a->size++;
  return 0;
}

/*
   Returns the number of entries in the array.
*/
static inline size_t GLUE3(array_, prefix, _size)(const TYPE *a) {
  ASSERT(a != NULL);
  return a->size;
}

/*
   Returns the capacity in the array until a resize is needed.
*/
static inline size_t GLUE3(array_, prefix, _capacity)(const TYPE *a) {
  return a->capacity;
}

#define HEAP_PARENT(x) (((x) -1) / 2)
#define HEAP_LEFT_CHILD(x) (2 * (x) + 1)
#define HEAP_RIGHT_CHILD(x) (2 * (x) + 2)

/* Restores the heap order below pos, assuming both subtrees of pos are
   already heaps.  The entry at pos is moved down, toward the larger child,
   until it fits. */
static inline void GLUE3(array_, prefix, _sift_down)(TYPE *a, size_t pos) {
  data_t last = a->data[pos];

  while (true) {
    size_t child = HEAP_LEFT_CHILD(pos);
    if (child >= a->size) {
      break;
    }
    if (child + 1 < a->size &&
        a->comp(&a->data[child + 1], &a->data[child]) > 0) {
      child++;
    }
    if (a->comp(&a->data[child], &last) <= 0) {
      break;
    }
    a->data[pos] = a->data[child];
    pos = child;
  }
  a->data[pos] = last;
}

/*
   Viewing the array as a heap, pushes a value onto the heap.

   Using array_prefix_heappush() and array_prefix_heappop()
   you get a heap.
*/

static inline int32_t GLUE3(array_, prefix, _heappush)(TYPE *a, data_t value) {
  if (a == NULL) {
    return -1;
  }
  if (a->comp == NULL) {
    return -1;
  }
  ASSERT(a->comp != NULL);
  if (GLUE3(array_, prefix, _append)(a, value) != 0) {
    return -1;
  }
  /* move a hole up from the new last slot instead of swapping at every level */
  size_t pos = a->size - 1;
  while (pos != 0) {
    size_t parent = HEAP_PARENT(pos);
    if (a->comp(&value, &(a->data[parent])) <= 0) {
      break;
    }
    a->data[pos] = a->data[parent];
    pos = parent;
  }
  a->data[pos] = value;
  return 0;
}

/*

   Viewing the array as a heap, pops the top value off the heap

*/

static inline data_t GLUE3(array_, prefix, _heappop)(TYPE *a) {
  ASSERT(a != NULL);
  ASSERT(a->comp != NULL);
  if (a->size <= 0) {
    if (a->have_null_value) {
      return a->null_value;
    } else {
      ASSERT(a->size > 0);
    }
  }
  data_t rv = a->data[0];
  a->size--;

  if (a->size > 0) {
    a->data[0] = a->data[a->size];
    GLUE3(array_, prefix, _sift_down)(a, 0);
  }
  return rv;
}

/*
   Heapifies an array in O(n) time.
*/

static inline int32_t GLUE3(array_, prefix, _heapify)(TYPE *a) {
  if (a == NULL) {
    return -1;
  }
  if (a->comp == NULL) {
    return -1;
  }
  /* Floyd's construction: sift down every entry that has a child, last to
     first.  This takes O(n) comparisons, where pushing each entry in turn
     takes O(n log n) in the worst case. */
  for (size_t i = a->size / 2; i > 0; i--) {
    GLUE3(array_, prefix, _sift_down)(a, i - 1);
  }
  return 0;
}

static inline ssize_t GLUE3(array_, prefix, _index)(const TYPE *a, data_t v) {
  ASSERT(a != NULL);
  ASSERT(a->comp != NULL);
  for (size_t i = 0; i < a->size; i++) {
    if (a->comp(&a->data[i], &v) == 0) {
      return i;
    }
  }
  return -1;
}

#define ARRAY_STR_HELPER(x) #x
#define ARRAY_STR(x) ARRAY_STR_HELPER(x)

static inline int32_t GLUE3(array_, prefix, _serialize)(const TYPE *a,
                                                        const char *filename) {
  if (a == NULL || filename == NULL) {
    return -1;
  }

  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    return -1;
  }

  const char *header = "Array===";
  fwrite(header, sizeof(char), 8, fp);
  serialize_string(ARRAY_STR(prefix), fp);

  const size_t data_size = sizeof(data_t);
  fwrite(&data_size, sizeof(size_t), 1, fp);

  const size_t arr_size = a->size;
  fwrite(&arr_size, sizeof(size_t), 1, fp);
  fwrite(a->data, data_size, arr_size, fp);

  /* stdio errors are sticky, so one check after all the writes covers them */
  int32_t rc = (fflush(fp) != 0 || ferror(fp)) ? -1 : 0;
  if (fclose(fp) != 0) {
    rc = -1;
  }
  return rc;
}

static inline TYPE *GLUE3(array_, prefix, _deserialize)(const char *filename) {
  if (filename == NULL) {
    return NULL;
  }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    return NULL;
  }

  TYPE *a = NULL;
  char *prefix_str = NULL;

  char header[8];
  if (fread(header, sizeof(char), 8, fp) != 8 ||
      strncmp(header, "Array===", 8) != 0) {
    goto fail;
  }

  prefix_str = deserialize_string(fp);
  if (prefix_str == NULL || strcmp(prefix_str, ARRAY_STR(prefix)) != 0) {
    goto fail;
  }

  size_t data_size;
  if (fread(&data_size, sizeof(size_t), 1, fp) != 1 ||
      data_size != sizeof(data_t)) {
    goto fail;
  }

  size_t arr_size;
  if (fread(&arr_size, sizeof(size_t), 1, fp) != 1) {
    goto fail;
  }

  a = GLUE3(array_, prefix, _init)();
  if (a == NULL) {
    goto fail;
  }

  if (arr_size > 0) {
    if (arr_size > SIZE_MAX / data_size) {
      goto fail;
    }
    data_t *buf = malloc(arr_size * data_size);
    if (buf == NULL) {
      goto fail;
    }
    free(a->alloc);
    a->alloc = buf;
    a->data = buf;
    a->capacity = arr_size;

    if (fread(a->data, data_size, arr_size, fp) != arr_size) {
      goto fail;
    }
    a->size = arr_size;
  }

  free(prefix_str);
  fclose(fp);
  return a;

fail:
  free(prefix_str);
  if (a != NULL) {
    GLUE3(array_, prefix, _destroy)(&a);
  }
  fclose(fp);
  return NULL;
}

#undef ARRAY_STR
#undef ARRAY_STR_HELPER
#undef HEAP_RIGHT_CHILD
#undef HEAP_LEFT_CHILD
#undef HEAP_PARENT

#undef TYPE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting)
   are removed so that they do not leak into the includer */
#undef _unused
#undef default_null_value
