#include <stdio.h>

#define data_t float
#define prefix float
#include <linked_list.h>
#undef prefix
#undef data_t

int main(void) {
  llist_float_t *a = llist_float_init();

  for (int i = 1; i <= 10; i++) {
    llist_float_add_start(a, 1.0 / i);
  }

  for (int i = 1; i <= 10; i++) {
    llist_float_add_end(a, 1.0 / i);
  }


  for (lnode_float_t *n = a->head; n != NULL; n = n->next) {
    printf("%12.4f \n", n->data);
  }
  llist_float_destroy(&a);
  return 0;
}
