#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cs_assert.h"
#include "cs_private.h"

#include "comp.h"
#include "cmp_hook.h"

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#ifndef null_value
#error "null_value not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define LLIST GLUE3(llist_, prefix, _t)
#define LNODE GLUE3(lnode_, prefix, _t)

typedef struct LNODE {
  data_t data;
  struct LNODE *next;
  struct LNODE *prev;
} LNODE;

typedef struct {
  LNODE *head;
  LNODE *tail;
  size_t size;
  int (*comp)(data_t *, data_t *);
} LLIST;

/* ----------------------------------------------------------------------- */
/*                  constructors / destructor                              */
/* ----------------------------------------------------------------------- */

#define _unused(x) ((void) (x))

static inline LLIST *GLUE3(llist_, prefix, _init)(void) {
  LLIST *a = malloc(sizeof(LLIST));
  if (a == NULL) {
    return a;
  }

  a->head = NULL;
  a->tail = NULL;
  a->size = 0;

  data_t temp;
  a->comp = DEFAULT_COMP(temp);
  _unused(temp);
  _unused(DEFAULT_COMP_TYPE(temp));

  return a;
}

/* int32_t llist_prefix_set_comp(llist_prefix_t *a,
                                 int (*comp)(data_t *, data_t *));

   Sets the comparison function used by llist_prefix_msort().

   return value:
     -1 => error (a is NULL; nothing is changed)
      0 => ok
*/
static inline int32_t
GLUE3(llist_, prefix, _set_comp)(LLIST *a, int (*comp)(data_t *, data_t *)) {
  if (a == NULL) {
    return -1;
  }
  a->comp = comp;
  return 0;
}

static inline void GLUE3(llist_, prefix, _destroy)(LLIST **a_ptr) {
  if (a_ptr == NULL) {
    return;
  }
  LLIST *a = *a_ptr;
  if (a == NULL) {
    return;
  }

  LNODE *n = a->head;
  while (n != NULL) {
    LNODE *temp = n->next;
    n->next = NULL;
    n->prev = NULL;
    free(n);
    n = temp;
  }

  a->head = NULL;
  a->tail = NULL;
  a->size = 0;
  a->comp = NULL;
  free(a);
  *a_ptr = NULL;
}

/* ----------------------------------------------------------------------- */
/*                          add / remove                                   */
/* ----------------------------------------------------------------------- */

/* size_t llist_prefix_size(const llist_prefix_t *a)

   Returns the number of entries in the list, or 0 if a is NULL.
*/
static inline size_t GLUE3(llist_, prefix, _size)(const LLIST *a) {
  return a == NULL ? 0 : a->size;
}

static inline int32_t GLUE3(llist_, prefix, _add_start)(LLIST *a, data_t d) {
  if (a == NULL) {
    return -1;
  }

  LNODE *n = malloc(sizeof(LNODE));
  if (n == NULL) {
    return -1;
  }
  n->data = d;
  n->next = a->head;
  n->prev = NULL;

  if (a->head == NULL) {
    a->head = n;
    a->tail = n;
    a->size = 1;
  } else {
    a->head->prev = n;
    a->head = n;
    a->size += 1;
  }

  return 0;
}

static inline int32_t GLUE3(llist_, prefix, _add_end)(LLIST *a, data_t d) {
  if (a == NULL) {
    return -1;
  }

  LNODE *n = malloc(sizeof(LNODE));
  if (n == NULL) {
    return -1;
  }
  n->data = d;
  n->next = NULL;
  n->prev = a->tail;

  if (a->tail == NULL) {
    a->head = n;
    a->tail = n;
    a->size = 1;
  } else {
    a->tail->next = n;
    a->tail = n;
    a->size += 1;
  }

  return 0;
}

static inline data_t GLUE3(llist_, prefix, _remove_start)(LLIST *a) {
  if (a == NULL || a->head == NULL) {
    return null_value;
  }

  LNODE *n = a->head;
  data_t rv = n->data;
  a->head = n->next;
  if (a->head == NULL) {
    a->tail = NULL;
  } else {
    a->head->prev = NULL;
  }
  a->size -= 1;
  free(n);
  return rv;
}

static inline data_t GLUE3(llist_, prefix, _remove_end)(LLIST *a) {
  if (a == NULL || a->head == NULL) {
    return null_value;
  }

  LNODE *n = a->tail;
  data_t rv = n->data;
  a->tail = n->prev;
  if (a->tail == NULL) {
    a->head = NULL;
  } else {
    a->tail->next = NULL;
  }
  a->size -= 1;
  free(n);
  return rv;
}

/* ----------------------------------------------------------------------- */
/*                     walk the linked list                                */
/* ----------------------------------------------------------------------- */

/* Walks over the list.  The iterator is a node pointer (lnode_prefix_t *), a
   plain variable owned by the caller: nothing is allocated, and the editing
   functions below (remove_forward, insert_before, ...) take the same pointer.
   The functions return

     0 => the iterator is at an entry, and *value (may be NULL) holds it
     1 => there is no entry (the end of the list, or an empty list); the
   iterator is NULL.  This is also the result if a or the iterator pointer
   is NULL.

   so a loop is

     lnode_prefix_t *it;
     for (int32_t rc = llist_prefix_first(a, &it, &value); rc == 0;
          rc = llist_prefix_next(&it, &value)) {
       ...
     }

   and llist_prefix_last / llist_prefix_prev walk from the end backwards.

   The list is not const: the node pointer handed out can be used to change
   the list (llist_prefix_insert_before and so on).
*/
static inline int32_t GLUE3(llist_, prefix, _first)(LLIST *a, LNODE **n_ptr,
                                                    data_t *value) {
  if (n_ptr == NULL) {
    return 1;
  }
  LNODE *n = a == NULL ? NULL : a->head;
  *n_ptr = n;
  if (n == NULL) {
    return 1;
  }
  if (value != NULL) {
    *value = n->data;
  }
  return 0;
}

static inline int32_t GLUE3(llist_, prefix, _last)(LLIST *a, LNODE **n_ptr,
                                                   data_t *value) {
  if (n_ptr == NULL) {
    return 1;
  }
  LNODE *n = a == NULL ? NULL : a->tail;
  *n_ptr = n;
  if (n == NULL) {
    return 1;
  }
  if (value != NULL) {
    *value = n->data;
  }
  return 0;
}

static inline int32_t GLUE3(llist_, prefix, _next)(LNODE **n_ptr,
                                                   data_t *value) {
  if (n_ptr == NULL) {
    return 1;
  }
  LNODE *n = *n_ptr;
  if (n != NULL) {
    n = n->next;
  }
  *n_ptr = n;
  if (n == NULL) {
    return 1;
  }
  if (value != NULL) {
    *value = n->data;
  }
  return 0;
}

static inline int32_t GLUE3(llist_, prefix, _prev)(LNODE **n_ptr,
                                                   data_t *value) {
  if (n_ptr == NULL) {
    return 1;
  }
  LNODE *n = *n_ptr;
  if (n != NULL) {
    n = n->prev;
  }
  *n_ptr = n;
  if (n == NULL) {
    return 1;
  }
  if (value != NULL) {
    *value = n->data;
  }
  return 0;
}

/* ----------------------------------------------------------------------- */
/*                     insert/delete nodes                                 */
/* ----------------------------------------------------------------------- */

/* Unlinks n from the list, frees it, and returns its value. */
CS_PRIVATE static inline data_t GLUE3(llist_, prefix, _unlink)(LLIST *a,
                                                               LNODE *n) {
  if (n->prev == NULL) {
    a->head = n->next;
  } else {
    n->prev->next = n->next;
  }
  if (n->next == NULL) {
    a->tail = n->prev;
  } else {
    n->next->prev = n->prev;
  }
  a->size -= 1;

  data_t rv = n->data;
  free(n);
  return rv;
}

/* Removes the current LNODE and moves the n_ptr to the next LNODE */
static inline data_t GLUE3(llist_, prefix, _remove_forward)(LLIST *a,
                                                            LNODE **n_ptr) {
  if (a == NULL || n_ptr == NULL || *n_ptr == NULL) {
    return null_value;
  }
  LNODE *n = *n_ptr;
  *n_ptr = n->next;
  return GLUE3(llist_, prefix, _unlink)(a, n);
}

/* Removes the current LNODE and moves the n_ptr to the previous LNODE */
static inline data_t GLUE3(llist_, prefix, _remove_backwards)(LLIST *a,
                                                              LNODE **n_ptr) {
  if (a == NULL || n_ptr == NULL || *n_ptr == NULL) {
    return null_value;
  }
  LNODE *n = *n_ptr;
  *n_ptr = n->prev;
  return GLUE3(llist_, prefix, _unlink)(a, n);
}

static inline int32_t GLUE3(llist_, prefix,
                            _insert_before)(LLIST *a, LNODE **n_ptr, data_t d) {
  if (a == NULL || n_ptr == NULL || *n_ptr == NULL) {
    return -1;
  }
  LNODE *n = *n_ptr;

  LNODE *new_node = malloc(sizeof(LNODE));
  if (new_node == NULL) {
    return -1;
  }
  new_node->data = d;
  new_node->next = n;
  new_node->prev = n->prev;
  n->prev = new_node;
  if (new_node->prev == NULL) {
    a->head = new_node;
  } else {
    new_node->prev->next = new_node;
  }
  a->size += 1;

  *n_ptr = new_node;
  return 0;
}

static inline int32_t GLUE3(llist_, prefix,
                            _insert_after)(LLIST *a, LNODE **n_ptr, data_t d) {
  if (a == NULL || n_ptr == NULL || *n_ptr == NULL) {
    return -1;
  }
  LNODE *n = *n_ptr;

  LNODE *new_node = malloc(sizeof(LNODE));
  if (new_node == NULL) {
    return -1;
  }
  new_node->data = d;
  new_node->next = n->next;
  new_node->prev = n;
  n->next = new_node;
  if (new_node->next == NULL) {
    a->tail = new_node;
  } else {
    new_node->next->prev = new_node;
  }
  a->size += 1;

  *n_ptr = new_node;
  return 0;
}

/* ----------------------------------------------------------------------- */
/*                        sort nodes                                       */
/* ----------------------------------------------------------------------- */

CS_PRIVATE static inline LNODE *
GLUE3(llist_, prefix, _merge_nodes)(LLIST *a, LNODE *x, LNODE *y) {
  _unused(a); /* with data_less defined, CS_CMP does not use a->comp */
  LNODE head;
  LNODE *tail = &head;

  while (x != NULL && y != NULL) {
    if (CS_CMP(a->comp, &y->data, &x->data) < 0) {
      tail->next = y;
      y = y->next;
    } else {
      tail->next = x;
      x = x->next;
    }
    tail = tail->next;
  }
  tail->next = x != NULL ? x : y;
  return head.next;
}

/* Sorts the singly linked chain of len nodes starting at n and returns the new
 * first node. */
CS_PRIVATE static inline LNODE *
GLUE3(llist_, prefix, _msort_nodes)(LLIST *a, LNODE *n, size_t len) {
  if (len < 2) {
    return n;
  }

  size_t half = len / 2;
  LNODE *mid = n;
  for (size_t i = 1; i < half; i++) {
    mid = mid->next;
  }
  LNODE *right = mid->next;
  mid->next = NULL;

  return GLUE3(llist_, prefix, _merge_nodes)(
      a, GLUE3(llist_, prefix, _msort_nodes)(a, n, half),
      GLUE3(llist_, prefix, _msort_nodes)(a, right, len - half));
}

/* int32_t llist_prefix_msort(llist_prefix_t *a);

   Sorts the list in place with a merge sort using the comparison
   function (see llist_prefix_set_comp).  The sort is stable, takes
   O(n log n) time in every case, needs O(log n) stack, and does not
   allocate.

   return value:
     -1 => error (a is NULL, or no comparison function has been set)
      0 => ok
*/
static inline int32_t GLUE3(llist_, prefix, _msort)(LLIST *a) {
  if (a == NULL || !CS_HAVE_CMP(a->comp)) {
    return -1;
  }
  if (a->size < 2) {
    return 0;
  }

  a->head = GLUE3(llist_, prefix, _msort_nodes)(a, a->head, a->size);

  LNODE *prev = NULL;
  for (LNODE *n = a->head; n != NULL; prev = n, n = n->next) {
    n->prev = prev;
  }
  a->tail = prev;
  return 0;
}

static inline int32_t GLUE3(llist_, prefix, _add_node)(LLIST *a, LNODE *n) {
  if (a == NULL || n == NULL) {
    return -1;
  }
  if (a->head == NULL) {
    n->prev = NULL;
    n->next = NULL;

    a->head = n;
    a->tail = n;
    a->size = 1;
  } else {
    n->prev = a->tail;
    a->tail->next = n;
    n->next = NULL;

    a->tail = n;
    a->size += 1;
  }
  return 0;
}

#undef LNODE
#undef LLIST
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting)
   are removed so that they do not leak into the includer */
#undef _unused
#undef null_value
#undef CS_CMP
#undef CS_HAVE_CMP
#undef CS_ASSERT_CMP
#undef data_less
