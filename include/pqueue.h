// binary heap backed priority queue
// https://algs4.cs.princeton.edu/24pq/

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

#define PQUEUE GLUE3(pqueue_, prefix, _t)
#define PNODE GLUE3(pnode_, prefix, _t)
#define PQKV GLUE3(pqkv_, prefix, _t)

#define PQUEUE_PARENT(x) (((x)-1)/2)
#define PQUEUE_LEFT_CHILD(x) (2*(x) + 1)
#define PQUEUE_RIGHT_CHILD(x) (2*(x) + 2)

/* ----------------------------------------------------------------------- */
/*                         data structures                                 */
/* ----------------------------------------------------------------------- */

typedef struct {
    data_t key;
    void *value;
} PNODE;

typedef struct {
    PNODE *data;
    size_t size;
    size_t capacity;
    int (*comp) (data_t *, data_t *);
    void (*value_free) (void *);
} PQUEUE;

typedef struct {
    data_t key;
    void *value;
    bool found;
} PQKV;

/* ----------------------------------------------------------------------- */
/*                         constructors                                    */
/* ----------------------------------------------------------------------- */

#define _unused(x) ((void)(x))

/* pqueue_prefix_t* pqueue_prefix_init();

   allocates and returns an empty priority queue.  The caller must
   free the returned pointer by calling pqueue_prefix_destroy(&q).

   By default, the priority queue is a max-heap: pqueue_prefix_pop()
   returns the key/value pair with the largest key according to the
   comp function.  To get min-first behavior, supply a comp function
   via pqueue_prefix_set_comp() which reverses the usual ordering.
*/
PQUEUE *GLUE3(pqueue_, prefix, _init) () {
    PQUEUE *q = malloc(sizeof(PQUEUE));
    if (q == NULL) {
        return q;
    }

    q->data = malloc(sizeof(PNODE));
    if (q->data == NULL) {
        free(q);
        return NULL;
    }
    q->size = 0;
    q->capacity = 1;
    data_t temp;
    q->comp = DEFAULT_COMP(temp);
    _unused(temp);
    q->value_free = NULL;

    _unused(DEFAULT_COMP_TYPE(temp));

    return q;
}

/* void pqueue_prefix_set_comp(pqueue_prefix_t *q, int (*comp) (data_t *, data_t *));

   Attaches a comparison function to the priority queue.  For basic
   data_t such as int8_t, int16_t, int32_t, int64_t, float, double,
   char*, this is not needed.  For more complicated data_t, the user
   must define the comparison function.  See comp.h for details and
   the C11 generics which deal with the basic data_ts.
*/
void GLUE3(pqueue_, prefix, _set_comp) (PQUEUE *q, int (*comp) (data_t *, data_t *)) {
    q->comp = comp;
}

/* void pqueue_prefix_set_value_free(pqueue_prefix_t *q, void (*value_free)(void *));

   Attaches a free function for the values held in the queue.  This is
   used when pqueue_prefix_destroy() is called, to free the value of
   every remaining entry.  The most common usage is to pass the system
   free() for the value_free assuming the values were allocated by the
   system malloc().
*/
void GLUE3(pqueue_, prefix, _set_value_free) (PQUEUE *q, void (*value_free) (void *)) {
    q->value_free = value_free;
}

/* ----------------------------------------------------------------------- */
/*                         destructor                                      */
/* ----------------------------------------------------------------------- */

/* void pqueue_prefix_destroy(pqueue_prefix_t **q_ptr);

   Destroys a priority queue, applies a user provided value_free
   function to the value of every remaining entry, frees the
   underlying storage, and sets the pointer to NULL.
*/
void GLUE3(pqueue_, prefix, _destroy) (PQUEUE **q_ptr) {
    PQUEUE *q = *q_ptr;
    // in case a user tries a double destroy
    if (q == NULL) {
        return;
    }

    if (q->value_free != NULL) {
        for (size_t i = 0; i < q->size; i++) {
            q->value_free(q->data[i].value);
        }
    }

    free(q->data);
    q->data = NULL;
    q->size = 0;
    q->capacity = 0;
    q->comp = NULL;
    q->value_free = NULL;
    free(q);
    *q_ptr = NULL;
}

/* ----------------------------------------------------------------------- */
/*                         operations                                      */
/* ----------------------------------------------------------------------- */

/* int32_t pqueue_prefix_push(pqueue_prefix_t *q, data_t key, void *value);

   Inserts a key/value pair into the priority queue.  Duplicate keys
   are allowed.

   return value:
     -1 => error (q is NULL or no comp function has been set)
      0 => ok
*/
int32_t GLUE3(pqueue_, prefix, _push) (PQUEUE *q, data_t key, void *value) {
    if (q == NULL || q->comp == NULL) {
        return -1;
    }

    if (q->size == q->capacity) {
        size_t new_capacity = q->capacity == 0 ? 1 : 2 * q->capacity;
        PNODE *tmp = malloc(new_capacity * sizeof(PNODE));
        assert(tmp != NULL);
        memcpy(tmp, q->data, q->capacity * sizeof(PNODE));
        free(q->data);
        q->data = tmp;
        q->capacity = new_capacity;
    }

    size_t pos = q->size;
    q->data[pos].key = key;
    q->data[pos].value = value;
    q->size++;

    while (pos != 0) {
        size_t parent = PQUEUE_PARENT(pos);
        if (q->comp(&(q->data[pos].key), &(q->data[parent].key)) <= 0) {
            break;
        }
        PNODE temp = q->data[parent];
        q->data[parent] = q->data[pos];
        q->data[pos] = temp;
        pos = parent;
    }
    return 0;
}

/* pqkv_prefix_t pqueue_prefix_pop(pqueue_prefix_t *q);

   Removes and returns the key/value pair at the top of the priority
   queue (the largest key, using the default comp).  If the queue is
   empty, the returned pqkv_prefix_t has found set to false.
*/
PQKV GLUE3(pqueue_, prefix, _pop) (PQUEUE *q) {
    assert(q != NULL);
    assert(q->comp != NULL);

    if (q->size == 0) {
        PQKV rv = {.value = NULL,.found = false };
        return rv;
    }

    PQKV rv = {.key = q->data[0].key,.value = q->data[0].value,.found = true };
    q->size--;

    if (q->size > 0) {
        q->data[0] = q->data[q->size];
        size_t pos = 0;

        while (true) {
            size_t left = PQUEUE_LEFT_CHILD(pos);
            size_t right = PQUEUE_RIGHT_CHILD(pos);

            if (left >= q->size && right >= q->size) {
                break;
            }

            if (right >= q->size) {
                if (q->comp(&q->data[left].key, &q->data[pos].key) > 0) {
                    PNODE temp = q->data[pos];
                    q->data[pos] = q->data[left];
                    q->data[left] = temp;
                }
                break;
            }

            if (q->comp(&q->data[pos].key, &q->data[left].key) > 0
                && q->comp(&q->data[pos].key, &q->data[right].key) > 0) {
                break;
            }

            size_t swap = q->comp(&q->data[right].key, &q->data[left].key) > 0 ? right : left;
            PNODE temp = q->data[pos];
            q->data[pos] = q->data[swap];
            q->data[swap] = temp;
            pos = swap;
        }
    }
    return rv;
}

/* pqkv_prefix_t pqueue_prefix_peek(const pqueue_prefix_t *q);

   Returns the key/value pair at the top of the priority queue without
   removing it.  If the queue is empty, the returned pqkv_prefix_t has
   found set to false.
*/
PQKV GLUE3(pqueue_, prefix, _peek) (const PQUEUE *q) {
    assert(q != NULL);

    if (q->size == 0) {
        PQKV rv = {.value = NULL,.found = false };
        return rv;
    }

    PQKV rv = {.key = q->data[0].key,.value = q->data[0].value,.found = true };
    return rv;
}

/*

  Returns the number of entries in the priority queue.
*/
size_t GLUE3(pqueue_, prefix, _size) (const PQUEUE *q) {
    assert(q != NULL);
    return q->size;
}

/*

  Returns true if the priority queue has no entries.
*/
bool GLUE3(pqueue_, prefix, _is_empty) (const PQUEUE *q) {
    assert(q != NULL);
    return q->size == 0;
}

#undef PQUEUE_RIGHT_CHILD
#undef PQUEUE_LEFT_CHILD
#undef PQUEUE_PARENT

#undef PQKV
#undef PNODE
#undef PQUEUE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
