#include <stdio.h>

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef prefix
#undef data_t

int main(void) {
  llist_int_t *a = llist_int_init();

  for (int i = 1; i <= 20; i++) {
    llist_int_add_end(a, i);
  }

  lnode_int_t *n;
  for (int d = llist_int_walk_init_start(a, &n);
       d != -1;
       d = llist_int_walk_forward(&n)) {
    if (d > 0 && (d % 3) == 0) {
      llist_int_insert_before(a, &n, -10);
      llist_int_walk_forward(&n);
    }
  }

  for (int d = llist_int_walk_init_start(a, &n);
       d != -1;
       d = llist_int_walk_forward(&n)) {
    if (d > 0 && (d % 5) == 0) {
      llist_int_insert_after(a, &n, -20);
    }
  }
  
  for (int d = llist_int_walk_init_start(a, &n);
       d != -1;
       d = llist_int_walk_forward(&n)) {
    printf("%d\n", d);
  }
  
  llist_int_destroy(&a);
  return 0;
}

    
