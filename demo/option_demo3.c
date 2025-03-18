#include <stdio.h>

#define data_t int
#define prefix int
#define sentinel_value (-1)
#include <option.h>
#undef sentinel_value
#undef prefix
#undef data_t


int main(void) {
  option_int_t x = option_int_init_empty();
  printf("%d\n", option_int_force_get(x));
  
  return 0;
}  

