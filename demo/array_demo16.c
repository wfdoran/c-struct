#define data_t int32_t
#define prefix int32
#include <array.h>
#undef prefix
#undef data_t



int main(void) {
  array_int32_t *a = array_int32_init();

  for (int32_t i = 0; i < 10; i++) {
    array_int32_append(a, i * i);
  }

  array_int32_serialize(a, "array_demo16.bin");

  array_int32_destroy(&a);
  return 0;
}

