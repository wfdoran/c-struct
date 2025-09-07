#include <stdlib.h>
#include <stdbool.h>

#include <comp.h>

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
    int (*comp) (data_t *, data_t *);
} LLIST;

/* 
  llist_prefix_size;
*/

/* ----------------------------------------------------------------------- */
/*                  constructors / destructor                              */
/* ----------------------------------------------------------------------- */

#define _unused(x) ((void) (x))

LLIST *GLUE3(llist_, prefix, _init) () {
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

void GLUE3(llist_, prefix, _set_comp) (LLIST *a, int (*comp) (data_t *, data_t *)) {
  a->comp = comp;
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
    a->comp = NULL;
    free(a);
    *a_ptr = NULL;
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

/* ----------------------------------------------------------------------- */
/*                     insert/delete nodes                                 */
/* ----------------------------------------------------------------------- */

/* Removes the current LNODE and moves the n_ptr to the next LNODE */
data_t GLUE3(llist_, prefix, _remove_forward) (LLIST *a, LNODE **n_ptr) {
    LNODE *n = *n_ptr;
    if (n == NULL) {
        return null_value;
    }

    /* Update the LLIST */
    if (n->prev == NULL) {
        a->head = n->next;
    }
    if (n->next == NULL) {
        a->tail = n->prev;
    }
    a->size -= 1;


    /* Update the neighbors */
    if (n->prev != NULL) {
        n->prev->next = n->next;
    }
    if (n->next != NULL) {
        n->next->prev = n->prev;
    }

    /* step and free this n */
    *n_ptr = n->next;
    data_t rv = n->data;
    n->data = null_value;
    n->prev = NULL;
    n->next = NULL;
    free(n);
    return rv;
}

data_t GLUE3(llist_, prefix, _remove_backwards) (LLIST *a, LNODE **n_ptr) {
    LNODE *n = *n_ptr;
    if (n == NULL) {
        return null_value;
    }

    /* Update the LLIST */
    if (n->prev == NULL) {
        a->head = n->next;
    }
    if (n->next == NULL) {
        a->tail = n->prev;
    }
    a->size -= 1;


    /* Update the neighbors */
    if (n->prev != NULL) {
        n->prev->next = n->next;
    }
    if (n->next != NULL) {
        n->next->prev = n->prev;
    }

    /* step and free this n */
    *n_ptr = n->prev;
    data_t rv = n->data;
    n->data = null_value;
    n->prev = NULL;
    n->next = NULL;
    free(n);
    return rv;
}

int32_t GLUE3(llist_, prefix, _insert_before) (LLIST *a, LNODE **n_ptr, data_t d) {
    LNODE *n = *n_ptr;
    if (n == NULL) {
        return -1;
    }

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

int32_t GLUE3(llist_, prefix, _insert_after) (LLIST *a, LNODE **n_ptr, data_t d) {
    LNODE *n = *n_ptr;
    if (n == NULL) {
        return -1;
    }

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

bool GLUE3(llist_, prefix, _sort_pass)(LLIST *a) {
  LNODE *n = a->head;
  bool change = false;

  while (n->next != NULL) {
    LNODE *m = n->next;

    if (a->comp(&m->data, &n->data) < 0) {
      if (n->prev == NULL) {
	a->head = m;
      } else {
	n->prev->next = m;
      }

      if (m->next == NULL) {
	a->tail = n;
      } else {
	m->next->prev = n;
      }
      
      m->prev = n->prev;
      n->next = m->next;

      m->next = n;
      n->prev = m;
      change = true;
    } else {
      n = m;
    }
  }
  return change;
}

void GLUE3(llist_, prefix, _sort)(LLIST *a) {
  if (a->size <= 1) {
    return;
  }
		   
  while (true) {
    bool change = GLUE3(llist_, prefix, _sort_pass)(a);
    if (!change) {
      break;
    }
  }
}

void GLUE3(llist_, prefix, _add_node)(LLIST *a, LNODE *n) {
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
}

void GLUE3(llist_, prefix, _qsort)(LLIST *a) {
  if (a->size <= 6) {
    GLUE3(llist_, prefix, _sort)(a);
    return;
  }

  LNODE *pivot = a->head;

  LLIST *sub_lo = GLUE3(llist_, prefix, _init)();
  LLIST *sub_hi = GLUE3(llist_, prefix, _init)();

  GLUE3(llist_, prefix, _set_comp)(sub_lo, a->comp);
  GLUE3(llist_, prefix, _set_comp)(sub_hi, a->comp);

  LNODE *save_next = NULL;
  for (LNODE *n = pivot->next; n != NULL; n = save_next) {
    save_next = n->next;
    if (a->comp(&pivot->data, &n->data) > 0) {
      GLUE3(llist_, prefix, _add_node)(sub_lo, n);
    } else {
      GLUE3(llist_, prefix, _add_node)(sub_hi, n);
    }
  }

  GLUE3(llist_, prefix, _qsort)(sub_lo);
  GLUE3(llist_, prefix, _qsort)(sub_hi);

  if (sub_lo->size == 0) {
    a->head = pivot;
    a->tail = sub_hi->tail;

    pivot->prev = NULL;
    pivot->next = sub_hi->head;
    sub_hi->head->prev = pivot;
  } else if (sub_hi->size == 0) {
    a->head = sub_lo->head;
    a->tail = pivot;

    pivot->next = NULL;
    pivot->prev = sub_lo->tail;
    sub_lo->tail->next = pivot;
  } else {
    a->head = sub_lo->head;
    a->tail = sub_hi->tail;

    pivot->prev = sub_lo->tail;
    sub_lo->tail->next = pivot;

    pivot->next = sub_hi->head;
    sub_hi->head->prev = pivot;
  }

  free(sub_lo);
  free(sub_hi);  
}


#undef LNODE
#undef LLIST
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
