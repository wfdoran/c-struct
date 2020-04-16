#include <stdlib.h>

#ifndef data_t
#error "data_t no defined"
#endif

#ifndef prefix
#error "prefix not defined"
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
  llist_prefix_add_start;
  llist_prefix_add_end;
  llist_prefix_remove_start;
  llist_prefix_remove_end;
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




#undef LNODE
#undef LLIST
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
