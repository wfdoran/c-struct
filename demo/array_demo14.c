#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define data_t int32_t
#define prefix int32
#include <array.h>
#undef prefix
#undef data_t



int main(void) {
    srand(time(NULL));

    for (int iter = 0; iter < 1000; iter++) {
      array_int32_t *a = array_int32_init();
      array_int32_set_null_value(a, -1);
      int n = 100 + rand() % 100;
      
      for (int i = 0; i < n; i++) {
          array_int32_heappush(a, rand() % 100);
      }

      int32_t x = array_int32_heappop(a);
      while (1) {
	int32_t y = array_int32_heappop(a);
	if (y == -1) {
	  break;
	}
	assert(y <= x);
	x = y;
      }
      array_int32_destroy(&a);
    }
}
