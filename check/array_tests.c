#include <check.h>
#include <stdlib.h>


#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

#define data_t char*
#define prefix str
#define default_null_value NULL
#include <array.h>
#undef default_null_value
#undef data_t
#undef prefix


typedef struct {
  int x;
  int y;
} pair_t;
    

#define data_t pair_t
#define prefix pair
#include <array.h>
#undef prefix
#undef data_t

static int comp_pair(pair_t *a, pair_t *b) {
  if (a->x < b->x) {
    return -1;
  }
  if (a->x > b->x) {
    return 1;
  }
  if (a->y < b->y) {
    return -1;
  }
  if (a->y > b->y) {
    return 1;
  }
  return 0;
}


char* free_str(char *s) {
  free(s);
  return NULL;
}

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

array_str_map(c, free_str);
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

START_TEST(array_test7)

char s1[] = "hello";
char s2[] = "world";
char s3[] = "all!";

array_str_t *a = array_str_init();
CHECK(a != NULL);
array_str_append(a, s1);
array_str_append(a, s2);
array_str_append(a, s3);

array_str_t *b = array_str_slice(a, 1, 2);
CHECK(b != NULL);

array_str_t *c = array_str_deep_slice(a, 1, 2, strdup);
CHECK(c != NULL);

s2[0] = 'W';

CHECK(strcmp(array_str_get(b,0), "World") == 0);
CHECK(strcmp(array_str_get(c,0), "world") == 0);

array_str_destroy(&a);
array_str_destroy(&b);
array_str_map(c, free_str);
array_str_destroy(&c);

CHECK(a == NULL);
CHECK(b == NULL);
CHECK(c == NULL);

END_TEST

START_TEST(array_test8)

array_int_t *a = array_int_init();
CHECK(a != NULL);
int null_value = -1;
array_int_set_null_value(a, null_value);
CHECK(array_int_pop(a) == null_value);
CHECK(array_int_pop_first(a) == null_value);
array_int_destroy(&a);
CHECK(a == NULL);

array_str_t *b = array_str_init();
CHECK(b != NULL);
CHECK(array_str_pop(b) == NULL);
CHECK(array_str_pop_first(b) == NULL);
array_str_destroy(&b);
CHECK(b == NULL);

END_TEST

START_TEST(array_test9)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
seed_rand(1);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  array_int_append(a, v);
}
array_int_sort(a);
for (int i = 0; i < n - 1; i++) {
  CHECK(array_int_get(a, i) <= array_int_get(a, i + 1));
}  
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(array_test10)

array_pair_t *a = array_pair_init();
CHECK(a != NULL);

int n = 10;
seed_rand(2);
for (int i = 0; i < n; i++) {
  pair_t q = {
    .x = (get_rand() >> 4) & 0xf,
    .y = (get_rand() >> 8) & 0xffff,
  };
  array_pair_append(a, q);
}
array_pair_set_comp(a, &comp_pair);
array_pair_sort(a);
for (int i = 0; i < n - 1; i++) {
  pair_t s = array_pair_get(a, i);
  pair_t t = array_pair_get(a, i + 1);
  CHECK(s.x < t.x || (s.x == t.x && s.y <= t.y));
}

array_pair_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test11)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
seed_rand(3);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  if ((v % 5) == 0) {
    v++;
  }
  array_int_append(a, v);
}
array_int_sort(a);

for (int i = 0; i < n; i++) {
  int v = array_int_get(a, i);
  CHECK(array_int_bisect(a, v) == i);
}

int mid = (array_int_get(a, 0) + array_int_get(a, n - 1)) / 2;
mid += (5 - (mid % 5));
CHECK(array_int_bisect(a, mid) == -1);

array_int_destroy(&a);
CHECK(a == NULL);
     
END_TEST

int square(int x) {
  return x * x;
}
int add(int x, int y) {
  return x + y;
}

START_TEST(array_test12)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 1; i <= n; i++) {
  array_int_append(a, i);
}
array_int_map(a, square);
int sum_sqrs = array_int_fold(a, add);

CHECK(sum_sqrs == n * (n + 1) * (2 * n + 1) / 6);

array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test13)

array_int_t *a = array_int_init();
CHECK(a != NULL);

array_int_t *b = array_int_init();
CHECK(a != NULL);


int n = 10;
seed_rand(4);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  array_int_append(a, v);
  array_int_heappush(b, v);
}
array_int_sort(a);

for (int i = 0; i < n; i++) {
  int v1 = array_int_get(a, n - 1 - i);
  int v2 = array_int_heappop(b);
  CHECK(v1 == v2);
}

CHECK(array_int_size(b) == 0);

array_int_destroy(&b);
array_int_destroy(&a);

END_TEST

