#include <check.h>
#include <stdlib.h>


#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

#define data_t char*
#define prefix str
#include <array.h>
#undef data_t
#undef data_t

START_TEST(array_test1)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test2)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_append(a, 5);
array_int_append(a, 6);
CHECK(array_int_size(a) == 2);
CHECK(array_int_pop(a) == 6);
CHECK(array_int_pop(a) == 5);
CHECK(array_int_size(a) == 0);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test3)

int default_value = -1;
size_t size = 8;
array_int_t *a = array_int_init2(size, default_value);
CHECK(a != NULL);
CHECK(array_int_size(a) == size);
for (int i = 0; i < size; i++) {
  CHECK(array_int_get(a, i) == default_value);
}
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test4)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_append(a, 1);
array_int_append(a, 2);
array_int_t *b = array_int_clone(a);
CHECK(b != NULL);
CHECK(array_int_size(a) == array_int_size(b));
for (int i = 0; i < array_int_size(a); i++) {
  CHECK(array_int_get(a, i) == array_int_get(b, i));
}
array_int_set(a, 5, 0);
CHECK(array_int_get(b, 0) == 1);
array_int_destroy(&a);
CHECK(a == NULL);
CHECK(b != NULL);
array_int_destroy(&b);
CHECK(b == NULL);

END_TEST


START_TEST(array_test5)

array_str_t *a = array_str_init();
CHECK(a != NULL);
char h[] = "Hello";
array_str_append(a, h);
array_str_t *b = array_str_clone(a);
CHECK(b != NULL);
array_str_t *c = array_str_deep_clone(a, strdup);

h[0] = 'B';

CHECK(strcmp(array_str_get(a, 0), "Bello") == 0);
CHECK(strcmp(array_str_get(b, 0), "Bello") == 0);
CHECK(strcmp(array_str_get(c, 0), "Hello") == 0);

array_str_destroy(&a);
CHECK(a == NULL);
array_str_destroy(&b);
CHECK(b == NULL);
array_str_destroy(&c);
CHECK(c == NULL);

END_TEST

START_TEST(array_test6)

array_int_t *a = array_int_init();
CHECK(a != NULL);
for (int i = 0; i < 5; i++) {
  array_int_append(a, i);
}
array_int_t *b = array_int_slice(a, 1, 4);
CHECK(b != NULL);

for (int i = 0; i < 5; i++) {
  array_int_set(a, i, -1);
}
for (int i = 0; i < 3; i++) {
  CHECK(array_int_get(b, i) == i + 1);
}
array_int_destroy(&a);
CHECK(a == NULL);
array_int_destroy(&b);
CHECK(b == NULL);

END_TEST

int main(void) {
  array_test1();
  array_test2();
  array_test3();
  array_test4();
  array_test5();
  array_test6();
  return 0;
}

