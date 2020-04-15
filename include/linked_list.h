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
  llist_prefix_destroy
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

#undef LNODE
#undef LLIST
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
