#include <check.h>

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef prefix
#undef data_t

#define data_t int
#define prefix dn
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
int x;
for (int32_t rc = llist_int_first(a, &nn, &x); rc == 0; rc = llist_int_next(&nn, &x)) {
  CHECK(x == y);
  y++;
}
CHECK(y == n);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test7)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < 10; i++) {
  llist_int_add_end(a, i);
}

int y = n;
lnode_int_t *nn = NULL;
int x;
for (int32_t rc = llist_int_last(a, &nn, &x); rc == 0; rc = llist_int_prev(&nn, &x)) {
  y--;
  CHECK(x == y);
}
CHECK(y == 0);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(linked_list_test8)

seed_rand(1);
for (int n = 0; n <= 200; n += 7) {
  llist_int_t *a = llist_int_init();
  CHECK(a != NULL);

  for (int i = 0; i < n; i++) {
    llist_int_add_end(a, get_rand() % 20);
  }

  CHECK(llist_int_msort(a) == 0);
  CHECK(a->size == (size_t) n);

  lnode_int_t *prev = NULL;
  int count = 0;
  for (lnode_int_t *x = a->head; x != NULL; prev = x, x = x->next) {
    CHECK(x->prev == prev);
    if (prev != NULL) {
      CHECK(prev->data <= x->data);
    }
    count++;
  }
  CHECK(count == n);
  CHECK(a->tail == prev);

  llist_int_destroy(&a);
  CHECK(a == NULL);
}

END_TEST

START_TEST(linked_list_test9)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);
CHECK(llist_int_size(a) == 0);

for (int i = 0; i < 10; i++) {
  llist_int_add_end(a, i);
}
CHECK(llist_int_size(a) == 10);
llist_int_add_start(a, -1);
CHECK(llist_int_size(a) == 11);

llist_int_remove_start(a);
llist_int_remove_end(a);
CHECK(llist_int_size(a) == 9);

lnode_int_t *n;
llist_int_first(a, &n, NULL);
llist_int_insert_after(a, &n, 100);
CHECK(llist_int_size(a) == 10);
llist_int_insert_before(a, &n, 101);
CHECK(llist_int_size(a) == 11);
llist_int_remove_forward(a, &n);
CHECK(llist_int_size(a) == 10);

CHECK(llist_int_msort(a) == 0);
CHECK(llist_int_size(a) == 10);

// removing everything, including from an empty list, never goes below zero
for (int i = 0; i < 12; i++) {
  llist_int_remove_start(a);
}
CHECK(llist_int_size(a) == 0);
llist_int_remove_end(a);
CHECK(llist_int_size(a) == 0);

llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(linked_list_test10)

llist_dn_t *l = NULL;
llist_dn_destroy(NULL);
llist_dn_destroy(&l);
CHECK(l == NULL);
l = llist_dn_init();
llist_dn_destroy(&l);
llist_dn_destroy(&l);
CHECK(l == NULL);

END_TEST

// first / last / next / prev: the end of the list, an empty list, and NULL for the value
START_TEST(linked_list_test11)

llist_int_t *a = llist_int_init();
lnode_int_t *n = (lnode_int_t *) 0x1;
int v = 99;
CHECK(llist_int_first(a, &n, &v) == 1);
CHECK(n == NULL && v == 99);
n = (lnode_int_t *) 0x1;
CHECK(llist_int_last(a, &n, &v) == 1);
CHECK(n == NULL && v == 99);
CHECK(llist_int_next(&n, &v) == 1);
CHECK(llist_int_prev(&n, &v) == 1);

for (int i = 1; i <= 3; i++) {
  llist_int_add_end(a, i);
}
CHECK(llist_int_first(a, &n, &v) == 0 && v == 1);
CHECK(llist_int_next(&n, &v) == 0 && v == 2);
CHECK(llist_int_next(&n, NULL) == 0);
CHECK(llist_int_next(&n, &v) == 1 && n == NULL);
CHECK(llist_int_next(&n, &v) == 1);

CHECK(llist_int_last(a, &n, &v) == 0 && v == 3);
CHECK(llist_int_prev(&n, &v) == 0 && v == 2);
CHECK(llist_int_prev(&n, &v) == 0 && v == 1);
CHECK(llist_int_prev(&n, &v) == 1 && n == NULL);

// the iterator is the node, so the editing functions work from it
CHECK(llist_int_first(a, &n, NULL) == 0);
CHECK(llist_int_next(&n, NULL) == 0);
CHECK(llist_int_remove_forward(a, &n) == 2);
CHECK(n != NULL && n->data == 3);
CHECK(llist_int_size(a) == 2);

llist_int_destroy(&a);

END_TEST
