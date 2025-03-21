#include <stdio.h>

#define data_t int
#define prefix int
#define sentinel_value (-1)
#include <option.h>
#undef sentinel_value
#undef prefix
#undef data_t

int square(int a) {
  return a * a;
}

int main(void) {
  option_int_t x = option_int_init(4);
  option_int_t y = x;
  option_int_map(&x, square);
  
  printf("%d\n", option_int_force_get(x));
  printf("%d\n", option_int_force_get(y));
  
  return 0;
}  

