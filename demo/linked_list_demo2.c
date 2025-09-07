#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef prefix
#undef data_t

int main(void) {
  srand48(time(NULL));
  
  llist_int_t *a = llist_int_init();

  for (int i = 0; i < 100; i++) {
    llist_int_add_start(a, lrand48() % 1000000);
  }

  lnode_int_t *n;
  for (int d = llist_int_walk_init_start(a, &n); n != NULL; d = llist_int_walk_forward(&n)) {
    printf("%8d\n", d);
  }
  printf("\n");

  
  llist_int_sort(a);

  for (int d = llist_int_walk_init_start(a, &n); n != NULL; d = llist_int_walk_forward(&n)) {
    printf("%8d\n", d);
  }
  printf("\n");

  

  
  llist_int_destroy(&a);
  return 0;
}
