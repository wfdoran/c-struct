#include <stdio.h>

#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix


int main(void) {
  array_int_t *a = array_int_init();

  for (int i = 1; i <= 10; i++) {
    array_int_append(a, i);
  }

  for (int i = 0; i < 5; i++) {
    int v1 = array_int_pop(a);
    printf("%d\n", v1);
    int v2 = array_int_pop_first(a);
    printf("%d\n", v2);
  }

  array_int_destroy(&a);
  return 0;
}

