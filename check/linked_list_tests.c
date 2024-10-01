#include <check.h>

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef prefix
#undef data_t

START_TEST(linked_list_test1)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);
llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test2)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  llist_int_add_start(a, i);
}

for (int i = 0; i < n; i++) {
  CHECK(llist_int_remove_end(a) == i);
}
CHECK(llist_int_remove_end(a) == -1);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test3)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  llist_int_add_end(a, i);
}

for (int i = 0; i < n; i++) {
  CHECK(llist_int_remove_start(a) == i);
}
CHECK(llist_int_remove_end(a) == -1);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test4)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  llist_int_add_start(a, i);
}

for (int i = 0; i < n; i++) {
  CHECK(llist_int_remove_start(a) == n - i - 1);
}
CHECK(llist_int_remove_end(a) == -1);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test5)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  llist_int_add_end(a, i);
}

for (int i = 0; i < n; i++) {
  CHECK(llist_int_remove_end(a) == n - i - 1);
}
CHECK(llist_int_remove_end(a) == -1);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test6)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < 10; i++) {
  llist_int_add_end(a, i);
}

int y = 0;
lnode_int_t *nn = NULL;
for (int x = llist_int_walk_init_start(a, &nn); x != -1; x = llist_int_walk_forward(&nn)) {
  CHECK(x == y);
  y++;
}
CHECK(y == n);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST
