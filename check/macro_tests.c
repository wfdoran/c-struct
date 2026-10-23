#include <check.h>
#include <stdint.h>
#include <sys/types.h>

// interval.h includes pqueue.h for its own use, with a data_t and prefix of its
// own.  It must leave the data_t, prefix and data_less the includer has defined
// for the next container as they were: the array below is instantiated after
// interval.h, with the macros defined before it.

#define data_t int
#define prefix mac
#define data_less(a, b) ((a) > (b))
#include <interval.h>

#ifndef data_t
#error "interval.h removed data_t"
#endif
#ifndef prefix
#error "interval.h removed prefix"
#endif
#ifndef data_less
#error "interval.h removed data_less"
#endif

#include <array.h>
#undef prefix
#undef data_t

// the macros reached array.h unchanged: it is ordered by the includer's
// data_less (descending), not by the double comparison of the interval queue
START_TEST(macro_test1)
array_mac_t *a = array_mac_init();
CHECK(array_mac_append(a, 1) == 0);
CHECK(array_mac_append(a, 3) == 0);
CHECK(array_mac_append(a, 2) == 0);
CHECK(array_mac_sort(a) == 0);
CHECK(array_mac_get(a, 0) == 3);
CHECK(array_mac_get(a, 1) == 2);
CHECK(array_mac_get(a, 2) == 1);
array_mac_destroy(&a);
END_TEST
