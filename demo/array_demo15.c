#include <stdio.h>
#include <string.h>

#define data_t char*
#define prefix str
#include <array.h>
#undef prefix
#undef data_t


int main(void) {
  
  array_str_t *a = array_str_init();

  array_str_append(a, "hello");
  array_str_append(a, "world");
  array_str_append(a, "this");
  array_str_append(a, "is");
  array_str_append(a, "a");
  array_str_append(a, "test");
  
  array_str_t *b = array_str_deep_slice(a, 2, 4, strdup);

  for (int i = 0; i < array_str_size(b); i++) {
    printf("%s\n", array_str_get(b, i));
  }
 
  array_str_destroy(&a);
  array_str_destroy(&b);
  return 0;
}
