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

  for (;;) {
    v = array_int_pop(a);
    if (v == null_value) {
      break;
    }
    printf("%d ", v);
    v = array_int_pop_first(a);
    if (v == null_value) {
      break;
    }
    printf("%d ", v);
  }
  printf("\n");

  for (int i = 1; i <= 10; i++) {
    array_int_append(a, i);
  }
  while((v = array_int_pop_first(a)) != null_value) {
    printf("%d ", v);
  }
  printf("\n");
  
  for (int i = 1; i <= 10; i++) {
    array_int_append(a, i);
  }
  while((v = array_int_pop(a)) != null_value) {
    printf("%d ", v);
  }
  printf("\n");
  
  
  for (int i = 0; i < 10; i++) {
    array_int_heappush(a, 1 + (7 * i) % 10 );
  }
  while((v = array_int_heappop(a)) != null_value) {
    printf("%d ", v);
  }
  printf("\n");
  
  
  array_int_destroy(&a);
  return 0;
}

