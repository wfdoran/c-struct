#include <stdlib.h>

#ifndef data_t
#error "data_t no defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#ifndef null_value
#error "null_value not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

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
} LLIST;

/* 
  llist_prefix_size;
  llist_prefix_set_start;
  llist_prefix_step_forward;
*/

/* ----------------------------------------------------------------------- */
/*                  constructors / destructor                              */
/* ----------------------------------------------------------------------- */

LLIST *GLUE3(llist_, prefix, _init) () {
  LLIST *a = malloc(sizeof(LLIST));
  if (a == NULL) {
    return a;
  }

  a->head = NULL;
  a->tail = NULL;
  a->size = 0;

  return a;
}

void GLUE3(llist_, prefix, _destroy) (LLIST **a_ptr) {
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
  free(a);
  a_ptr = NULL;
}

/* ----------------------------------------------------------------------- */
/*                          add / remove                                   */
/* ----------------------------------------------------------------------- */

int32_t GLUE3(llist_, prefix, _add_start) (LLIST *a, data_t d) {
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

int32_t GLUE3(llist_, prefix, _add_end) (LLIST *a, data_t d) {
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

  if (a->head == NULL) {
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

data_t GLUE3(llist_, prefix, _remove_start) (LLIST *a) {
  if (a == NULL || a->head == NULL) {
    return null_value;
  }

  LNODE *n = a->head;
  data_t rv = n->data;
  a->head = n->next;
  if (a->head == NULL) {
    a->tail = NULL;
  }
  a->size -= 1;
  free(n);
  return rv;
}

data_t GLUE3(llist_, prefix, _remove_end) (LLIST *a) {
  if (a == NULL || a->head == NULL) {
    return null_value;
  }

  LNODE *n = a->tail;
  data_t rv = n->data;
  a->tail = n->prev;
  if (a->tail == NULL) {
    a->head = NULL;
  }
  a->size -= 1;
  free(n);
  return rv;
}

/* ----------------------------------------------------------------------- */
/*                     walk the linked list                                */
/* ----------------------------------------------------------------------- */

data_t GLUE3(llist_, prefix, _walk_init_start) (LLIST *a, LNODE **n_ptr) {
  LNODE *n = a->head;
  *n_ptr = n;
  return n == NULL ? null_value : n->data;
}

data_t GLUE3(llist_, prefix, _walk_init_end) (LLIST *a, LNODE **n_ptr) {
  LNODE *n = a->tail;
  *n_ptr = n;
  return n == NULL ? null_value : n->data;
}

data_t GLUE3(llist_, prefix, _walk_forward) (LNODE **n_ptr) {
  LNODE *n = *n_ptr;
  if (n != NULL) {
    n = n->next;
  }
  *n_ptr = n;
  return n == NULL ? null_value : n->data;
}

data_t GLUE3(llist_, prefix, _walk_backwards) (LNODE **n_ptr) {
  LNODE *n = *n_ptr;
  if (n != NULL) {
    n = n->prev;
  }
  *n_ptr = n;
  return n == NULL ? null_value : n->data;
}
  

#undef LNODE
#undef LLIST
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
