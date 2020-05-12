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

  lnode_float_t *n;
  for (float d = llist_float_walk_init_start(a, &n);
       !isnan(d);
       d = llist_float_walk_forward(&n)) {
    printf("%12.4f \n", d);
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
