#include <stdlib.h>

#ifndef data_t
#error "data_t no defined"
#endif

typedef struct {
    data_t data;
    struct ll_node_t *next;
} ll_node_t;

typedef struct {
    ll_node_t *head;
    ll_node_t *tail;
    size_t length;
    int (*cmp) (data_t a, data_t b);
} linked_list_t;

#define INIT_LINKED_LIST_T { \
    .head = NULL, \
    .tail = NULL,  \
    .length = 0, \
    .cmp = NULL  \
}
    