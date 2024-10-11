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

START_TEST(chan_test2)

int n = 10;

chan_int_t *a = chan_int_init(n);
CHECK(a != NULL);

for (int i = 0; i < n; i++) {
  int32_t rc = chan_int_send(a, i);
  CHECK(rc == CHAN_SUCCESS);
}

for (int i = 0; i < n; i++) {
  int value;
  int32_t rc = chan_int_recv(a, &value);
  CHECK(rc == CHAN_SUCCESS);
  CHECK(value == i);
}

chan_int_destroy(&a);
CHECK(a == NULL);

END_TEST

