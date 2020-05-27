#include <stdio.h>

#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix


int main(void) {
  int v;
  int null_value = -1;

  array_int_t *a = array_int_init();
  array_int_set_null_value(a, null_value);
  

  for (int i = 1; i <= 10; i++) {
    array_int_append(a, i);
  }

  int *b = *(int**) a;
  int n = array_int_size(a);

  for (int i = 0; i < n; i++) {
    printf("%d ", b[i]);
  }
  printf("\n");
 
  
  array_int_destroy(&a);
  return 0;
}

