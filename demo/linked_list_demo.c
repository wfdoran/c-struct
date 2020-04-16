#include <stdio.h>
#include <math.h>

#define data_t float
#define prefix float
#define null_value NAN
#include <linked_list.h>
#undef null_value
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
  
  while (1) {
    float v = llist_float_remove_start(a);
    if (isnan(v)) {
      break;
    }
    printf("-- %.2f --\n", v);
  }
  
  llist_float_destroy(&a);
  return 0;
}
