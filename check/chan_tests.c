#include <check.h>


#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

START_TEST(chan_test1)

for (int chan_size = 1; chan_size <= 100; chan_size *= 10) {
  chan_int_t *a = chan_int_init(chan_size);
  CHECK(a != NULL);

  chan_int_destroy(&a);
  CHECK(a == NULL);
}

END_TEST
