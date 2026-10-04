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
  int d;
  for (int32_t rc = llist_int_first(a, &n, &d); rc == 0;
       rc = llist_int_next(&n, &d)) {
    printf("%8d\n", d);
  }
  printf("\n");

  if (llist_int_msort(a) != 0) {
    fprintf(stderr, "sort failed\n");
    return 1;
  }

  for (int32_t rc = llist_int_first(a, &n, &d); rc == 0;
       rc = llist_int_next(&n, &d)) {
    printf("%8d\n", d);
  }
  printf("\n");

  llist_int_destroy(&a);
  return 0;
}
