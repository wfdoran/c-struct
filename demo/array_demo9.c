#include <stdio.h>

#define data_t int
#define prefix int
#include <array.h>
#undef prefix
#undef data_t

void print_range(const array_int_t *a, int left, int right) {
  printf("%d %d:", left, right);
  int idx_lower = array_int_bisect_lower(a, left);
  int idx_upper = array_int_bisect_upper(a, right);
  
  for (int i = idx_lower; i <= idx_upper; i++) {
    printf(" %d", a->data[i]);
  }
  printf("\n");
}

int main(void) {
  array_int_t *a = array_int_init();

  array_int_append(a, 1);
  array_int_append(a, 2);
  array_int_append(a, 4);
  array_int_append(a, 4);
  array_int_append(a, 4);
  array_int_append(a, 7);
  array_int_append(a, 9);

  print_range(a, 0, 10);
  print_range(a, 2, 4);
  print_range(a, 3, 8);
  print_range(a, 5, 6);
  print_range(a, 10, 20);
  print_range(a, -5, 0);

  printf("\n");
  for (int v = 0; v <= 10; v++) {
    printf("%2d %2ld %2ld\n", v, array_int_bisect_lower(a, v), array_int_bisect_upper(a,v));
  }

  array_int_destroy(&a);
  
  return 0;
}
