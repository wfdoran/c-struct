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

  const char *filename = "array_demo16.bin";
  array_int32_serialize(a, filename);

  array_int32_t *b = array_int32_deserialize(filename);

  for (int32_t i = 0; i < 10; i++) {
    printf("%3d %8d %8d\n", i, array_int32_get(a, i), array_int32_get(b, i));
  }
  
  array_int32_destroy(&b);
  array_int32_destroy(&a);
  return 0;
}

