#include <stdio.h>

#define data_t int
#define prefix int
#include <option.h>
#undef prefix
#undef data_t

void option_int_print(option_int_t x) {
  int value;

  switch(option_int_get(x, &value)) {
  case true:
    printf("%d\n", value);
    break;
  case false:
    printf("empty\n");
    break;
  }
}

int main(void) {
  option_int_t x = option_int_init(5);
  option_int_print(x);
  int z;
  bool got_z = option_int_get_clear(&x, &z);
  if (got_z) {
    printf("%d\n", z);
  } else {
    printf("no z!");
  }
  option_int_print(x);
  printf("\n");

  option_int_t y = option_int_init_empty();
  printf("%d\n", option_int_get_or_else(y, -1));
  option_int_print(y);
  option_int_set(&y, 8);
  printf("%d\n", option_int_get_or_else(y, -1));  
  option_int_print(y);
  
  return 0;
}  

