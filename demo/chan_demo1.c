
#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

int main(void) {
  chan_int_t *c = chan_int_init(10);
  chan_int_destroy(&c);
  return 0;
}
