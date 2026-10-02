#include <check.h>
#include <pthread.h>
#include <time.h>


#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

#define data_t int
#define prefix dn
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

chan_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(chan_test4)

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
      int32_t rc;
      int i = 0;
      while (true) {
	if (i == n) {
	  /* the first close succeeds, a second close reports CHAN_CLOSED */
	  rc = chan_int_close(a);
	  if (rc != CHAN_SUCCESS) {
	    good_send = false;
	  }
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
      int32_t rc;
      int j = 0;
      while (true) {
	int value;
	rc = chan_int_tryrecv(a, &value);
	if (rc == CHAN_CLOSED) {
	  /* everything sent before the close must have been received */
	  if (j != n) {
	    good_recv = false;
	  }
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

chan_int_destroy(&a);
CHECK(a == NULL);

END_TEST

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(chan_test5)

chan_dn_t *c = NULL;
chan_dn_destroy(NULL);
chan_dn_destroy(&c);
CHECK(c == NULL);
c = chan_dn_init(1);
chan_dn_destroy(&c);
chan_dn_destroy(&c);
CHECK(c == NULL);

END_TEST

static void *chan_test6_receiver(void *p) {
  int v;
  chan_int_recv((chan_int_t *) p, &v);
  return NULL;
}

static double process_cpu_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return (double) ts.tv_sec + 1e-9 * (double) ts.tv_nsec;
}

// A receiver which waits for a long time must not burn a CPU while it waits
START_TEST(chan_test6)

chan_int_t *c = chan_int_init(4);
pthread_t t;
double cpu0 = process_cpu_seconds();
CHECK(pthread_create(&t, NULL, chan_test6_receiver, c) == 0);

struct timespec wait = {0, 500 * 1000 * 1000};
nanosleep(&wait, NULL);
double used = process_cpu_seconds() - cpu0;
CHECK(used < 0.25);   // about 0.5 s with a bare yield loop, about 0.03 s with the backoff

CHECK(chan_int_send(c, 7) == CHAN_SUCCESS);
CHECK(pthread_join(t, NULL) == 0);
chan_int_destroy(&c);

END_TEST
