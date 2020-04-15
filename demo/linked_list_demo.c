#define data_t float
#define prefix float
#include <linked_list.h>
#undef prefix
#undef data_t

int main(void) {
  llist_float_t *a = llist_float_init();
  llist_float_destroy(&a);
  return 0;
}
