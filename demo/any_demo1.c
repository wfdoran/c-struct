#include <stdio.h>

#include <any.h>

int main(void) {
  any_t a = any_init(5);
  any_t b = any_init(3.2);


  printf("%d %d\n", any_get_type(a), any_get_type(b));

  
  printf("%d\n", ANY_VALUE(a, i32));

  return 0;
}
