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

START_TEST(chan_test3)

int n = 10;
chan_int_t *a = chan_int_init(1);
CHECK(a != NULL);
bool good_send = true;
bool good_recv = true;

#pragma omp parallel
{
  #pragma omp sections
  {
    #pragma omp section
    for (int i = 0; i < n; i++) {
      int32_t rc = chan_int_send(a, i);
      if (rc != CHAN_SUCCESS) {
	good_send = false;
	break;
      }
    }

    #pragma omp section
    for (int i = 0; i < n; i++) {
      int value;
      int32_t rc = chan_int_recv(a, &value);
      if (rc != CHAN_SUCCESS || value != i) {
	good_recv = false;
	break;
      }
    }
  }  
}

CHECK(good_send);
CHECK(good_recv);

END_TEST

START_TEST(chan_test4)

int32_t rc;
int n = 10;
chan_int_t *a = chan_int_init(1);
CHECK(a != NULL);
bool good_send = true;
bool good_recv = true;

#pragma omp parallel
{
  #pragma omp sections
  {
    #pragma omp section
    {
      int i = 0;
      while (true) {
	if (i == n) {
	  rc = chan_int_close(a);
	  if (rc != CHAN_CLOSED) {
	    good_send = false;
	  }
	  break;
	}

	rc = chan_int_trysend(a, i);
	if (rc == CHAN_CLOSED) {
	  good_send = false;
	  break;
	}
	if (rc == CHAN_SUCCESS) {
	  i++;
	}
      }
    }

    #pragma omp section
    {
      int j = 0;
      while (true) {
	int value;
	rc = chan_int_tryrecv(a, &value);
	if (rc == CHAN_CLOSED) {
	  break;
	}
	if (rc == CHAN_SUCCESS) {
	  if (value != j) {
	    good_recv = false;
	    break;
	  }
	  j++;
	}
      }
    }
  }
}
	  

CHECK(good_send);
CHECK(good_recv);

END_TEST

