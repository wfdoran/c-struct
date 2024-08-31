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
  option_int_t y = option_int_init_empty();

  option_int_print(x);
  option_int_print(y);

  option_int_get_clear(&x, NULL);
  option_int_print(x);

  return 0;
}  

