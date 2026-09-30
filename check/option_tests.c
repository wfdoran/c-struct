#include <check.h>
#include <stdlib.h>
#include <string.h>

#define data_t int
#define prefix oi
#include <option.h>
#undef prefix
#undef data_t

#define data_t double
#define prefix od
#define sentinel_value (-1.0)
#include <option.h>
#undef prefix
#undef data_t

#define data_t char*
#define prefix os
#include <option.h>
#undef prefix
#undef data_t

static int plus_one(int x) {
  return x + 1;
}

// data_t is a macro, so the clone callback's parameter is 'const char *'
static char *copy_string(const char *s) {
  char *rv = malloc(strlen(s) + 1);
  strcpy(rv, s);
  return rv;
}

START_TEST(option_test1)

option_oi_t a = option_oi_init(5);
option_oi_t b = option_oi_init_empty();

CHECK(option_oi_is_set(a));
CHECK(!option_oi_is_set(b));

int v = 0;
CHECK(option_oi_get(a, &v));
CHECK(v == 5);

v = 77;
CHECK(!option_oi_get(b, &v));
CHECK(v == 77);

CHECK(option_oi_get(a, NULL));
CHECK(!option_oi_get(b, NULL));

END_TEST

START_TEST(option_test2)

option_oi_t a = option_oi_init(5);
option_oi_t b = option_oi_init_empty();

CHECK(option_oi_force_get(a) == 5);
CHECK(option_oi_get_or_else(a, 9) == 5);
CHECK(option_oi_get_or_else(b, 9) == 9);

option_oi_set(&b, 12);
CHECK(option_oi_is_set(b));
CHECK(option_oi_force_get(b) == 12);

option_oi_set(&a, -3);
CHECK(option_oi_force_get(a) == -3);

END_TEST

START_TEST(option_test3)

option_oi_t a = option_oi_init(5);
option_oi_t b = option_oi_init_empty();

int v = 0;
CHECK(option_oi_get_clear(&a, &v));
CHECK(v == 5);
CHECK(!option_oi_is_set(a));
CHECK(!option_oi_get_clear(&a, &v));

CHECK(!option_oi_get_clear(&b, NULL));

option_oi_t c = option_oi_init(8);
CHECK(option_oi_get_clear(&c, NULL));
CHECK(!option_oi_is_set(c));

END_TEST

START_TEST(option_test4)

option_oi_t a = option_oi_init(5);
option_oi_t b = option_oi_init_empty();

CHECK(option_oi_map(&a, plus_one) == 0);
CHECK(option_oi_force_get(a) == 6);
CHECK(option_oi_map(&b, plus_one) == 0);
CHECK(!option_oi_is_set(b));

CHECK(option_oi_map(NULL, plus_one) == -1);
CHECK(option_oi_map(&a, NULL) == -1);
CHECK(option_oi_force_get(a) == 6);

END_TEST

START_TEST(option_test5)

option_oi_t a = option_oi_init(5);
option_oi_t b = option_oi_clone(a);
CHECK(option_oi_is_set(b));
CHECK(option_oi_force_get(b) == 5);

option_oi_set(&a, 6);
CHECK(option_oi_force_get(b) == 5);

option_oi_t e = option_oi_init_empty();
option_oi_t f = option_oi_clone(e);
CHECK(!option_oi_is_set(f));

END_TEST

START_TEST(option_test6)

option_os_t a = option_os_init(copy_string("hello"));
option_os_t b = option_os_deep_clone(a, copy_string);
CHECK(option_os_is_set(b));
CHECK(option_os_force_get(b) != option_os_force_get(a));
CHECK(strcmp(option_os_force_get(b), "hello") == 0);

option_os_t shallow = option_os_clone(a);
CHECK(option_os_force_get(shallow) == option_os_force_get(a));

option_os_t e = option_os_init_empty();
option_os_t g = option_os_deep_clone(e, copy_string);
CHECK(!option_os_is_set(g));

free(option_os_force_get(a));
free(option_os_force_get(b));

END_TEST

// With a sentinel value, an empty option holds it and force_get returns it
// instead of asserting.
START_TEST(option_test7)

option_od_t a = option_od_init(2.5);
option_od_t e = option_od_init_empty();

CHECK(option_od_is_set(a));
CHECK(!option_od_is_set(e));
CHECK(e.value == -1.0);
CHECK(option_od_force_get(a) == 2.5);
CHECK(option_od_force_get(e) == -1.0);

double v = 0;
CHECK(option_od_get_clear(&a, &v));
CHECK(v == 2.5);
CHECK(!option_od_is_set(a));
CHECK(a.value == -1.0);

END_TEST
