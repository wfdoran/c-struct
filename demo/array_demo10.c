#include <stdio.h>
#include <string.h>

#define data_t char*
#define prefix str
#include <array.h>
#undef prefix
#undef data_t


char* my_strcat(char *a, const char *b) {
  char *rv = malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
  strcpy(rv, a);
  strcat(rv, b);
  free(a);
  return rv;
}

char* append_space(char *s) {
  char *rv = malloc((strlen(s) + 2) * sizeof(char));
  strcpy(rv, s);
  strcat(rv, " ");
  free(s);
  return rv;
}

int main(void) {

  array_str_t *a = array_str_init();

  array_str_append(a, "hello");
  array_str_append(a, "world");
  array_str_append(a, "this");
  array_str_append(a, "is");
  array_str_append(a, "a");
  array_str_append(a, "test");
  
  array_str_t *b = array_str_deep_clone(a, strdup);

  array_str_map(b, append_space);
  char *s = malloc(sizeof(char));
  s[0] = 0;
  s = array_str_fold2(b, s, my_strcat);
  printf("%s\n", s);

  array_str_destroy(&a);
  array_str_destroy(&b);
}
