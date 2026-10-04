#include <check.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
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

// An instantiation whose allocations can be made to fail, to test the out of memory paths.
static long coom_budget = -1;   // allocations which may still succeed; -1 for no limit

static __attribute__((noinline)) void *coom_malloc(size_t n) {
  if (coom_budget == 0) {
    return NULL;
  }
  if (coom_budget > 0) {
    coom_budget--;
  }
  return malloc(n);
}

static __attribute__((noinline)) void *coom_aligned_alloc(size_t align, size_t n) {
  if (coom_budget == 0) {
    return NULL;
  }
  if (coom_budget > 0) {
    coom_budget--;
  }
  return aligned_alloc(align, n);
}

#define malloc coom_malloc
#define aligned_alloc coom_aligned_alloc
#define data_t int
#define prefix oom
#include <chan.h>
#undef prefix
#undef data_t
#undef aligned_alloc
#undef malloc

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

// ---------------------------------------------------------------------------
// helpers for the threaded tests

typedef struct {
  chan_int_t *c;
  int n;
  int id;
  int rc;
} chan_producer_t;

// sends id * 100000 + 0, 1, ... n - 1
static void *chan_producer(void *p) {
  chan_producer_t *a = p;
  a->rc = CHAN_SUCCESS;
  for (int i = 0; i < a->n; i++) {
    if (chan_int_send(a->c, a->id * 100000 + i) != CHAN_SUCCESS) {
      a->rc = CHAN_ERROR;
      return NULL;
    }
  }
  return NULL;
}

typedef struct {
  chan_int_t *c;
  long count;
  long sum;
  int ordered;   // each producer's values arrived in the order they were sent
  int last[16];
} chan_consumer_t;

static void *chan_consumer(void *p) {
  chan_consumer_t *a = p;
  a->count = 0;
  a->sum = 0;
  a->ordered = 1;
  for (int i = 0; i < 16; i++) {
    a->last[i] = -1;
  }
  int v;
  while (chan_int_recv(a->c, &v) == CHAN_SUCCESS) {
    int id = v / 100000;
    int seq = v % 100000;
    if (id < 0 || id >= 16 || seq <= a->last[id]) {
      a->ordered = 0;
    } else {
      a->last[id] = seq;
    }
    a->count++;
    a->sum += v;
  }
  return NULL;
}

static void *chan_blocked_send(void *p) {
  chan_int_t *c = p;
  return (void *) (long) chan_int_send(c, 1);
}

static void *chan_blocked_recv(void *p) {
  chan_int_t *c = p;
  int v;
  return (void *) (long) chan_int_recv(c, &v);
}

static void short_sleep(long ms) {
  struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
  nanosleep(&ts, NULL);
}

// A channel holds exactly its capacity, whatever the size of the slot array behind it, and
// values come out in the order they went in, through many wrap-arounds of the positions.
START_TEST(chan_test7)

int64_t caps[] = {1, 2, 3, 5, 7, 8, 9, 31, 100, 1000};
seed_rand(7);
for (int ci = 0; ci < 10; ci++) {
  int64_t cap = caps[ci];
  chan_int_t *c = chan_int_init(cap);
  CHECK(c != NULL);
  int value;

  // exactly cap values fit
  for (int i = 0; i < cap; i++) {
    CHECK(chan_int_trysend(c, i) == CHAN_SUCCESS);
  }
  CHECK(chan_int_trysend(c, -1) == CHAN_FULL);
  CHECK(chan_int_tryrecv(c, &value) == CHAN_SUCCESS && value == 0);
  CHECK(chan_int_trysend(c, (int) cap) == CHAN_SUCCESS);   // room for exactly one more
  CHECK(chan_int_trysend(c, -1) == CHAN_FULL);
  // a receive which discards the value
  CHECK(chan_int_tryrecv(c, NULL) == CHAN_SUCCESS);
  for (int i = 2; i <= cap; i++) {
    CHECK(chan_int_tryrecv(c, &value) == CHAN_SUCCESS && value == i);
  }
  CHECK(chan_int_tryrecv(c, &value) == CHAN_EMPTY);

  // a long random run against a model
  int next_send = 0, next_recv = 0;
  for (int step = 0; step < 6000; step++) {
    int count = next_send - next_recv;
    if (get_rand() & 1) {
      int32_t rc = chan_int_trysend(c, next_send);
      if (count < cap) {
        CHECK(rc == CHAN_SUCCESS);
        next_send++;
      } else {
        CHECK(rc == CHAN_FULL);
      }
    } else {
      int32_t rc = chan_int_tryrecv(c, &value);
      if (count > 0) {
        CHECK(rc == CHAN_SUCCESS && value == next_recv);
        next_recv++;
      } else {
        CHECK(rc == CHAN_EMPTY);
      }
    }
  }
  chan_int_destroy(&c);
}

END_TEST

// init: a capacity below 1 is rounded up to 1, one which cannot be allocated gives NULL, and
// every call refuses a NULL channel
START_TEST(chan_test8)

int64_t small[] = {0, -1, -5, INT64_MIN};
for (int i = 0; i < 4; i++) {
  chan_int_t *c = chan_int_init(small[i]);
  CHECK(c != NULL);
  CHECK(chan_int_trysend(c, 1) == CHAN_SUCCESS);
  CHECK(chan_int_trysend(c, 2) == CHAN_FULL);     // exactly one value
  chan_int_destroy(&c);
}

CHECK(chan_int_init(INT64_MAX) == NULL);          // the slot array cannot be sized
CHECK(chan_int_init(INT64_C(1) << 62) == NULL);   // more than a size_t of values

int v = 0;
CHECK(chan_int_trysend(NULL, 1) == CHAN_ERROR);
CHECK(chan_int_tryrecv(NULL, &v) == CHAN_ERROR);
CHECK(chan_int_send(NULL, 1) == CHAN_ERROR);
CHECK(chan_int_recv(NULL, &v) == CHAN_ERROR);
CHECK(chan_int_close(NULL) == CHAN_ERROR);

chan_int_t *c = chan_int_init(4);
chan_int_destroy(&c);
CHECK(c == NULL);
chan_int_destroy(&c);       // already destroyed
chan_int_destroy(NULL);

// the wait helper yields every 64th call and starts counting again
int32_t spins = 0;
for (int i = 1; i < 64; i++) {
  chan_wait(&spins);
  CHECK(spins == i);
}
chan_wait(&spins);
CHECK(spins == 0);

// the backoff counts its attempts up to a limit and stays there
int32_t attempts = 0;
for (int i = 1; i <= 100; i++) {
  chan_backoff(&attempts);
  CHECK(attempts == i);
}
for (int i = 0; i < CHAN_BACKOFF_YIELDS + 100; i++) {
  chan_backoff(&attempts);
}
CHECK(attempts == CHAN_BACKOFF_YIELDS);

END_TEST

// out of memory: the channel and then its slot array can each fail to allocate
START_TEST(chan_test9)

coom_budget = 0;
CHECK(chan_oom_init(8) == NULL);
coom_budget = 1;                  // the channel is allocated, the slots are not
CHECK(chan_oom_init(8) == NULL);
coom_budget = 2;
chan_oom_t *c = chan_oom_init(8);
coom_budget = -1;
CHECK(c != NULL);
CHECK(chan_oom_trysend(c, 5) == CHAN_SUCCESS);
int v;
CHECK(chan_oom_tryrecv(c, &v) == CHAN_SUCCESS && v == 5);
chan_oom_destroy(&c);

END_TEST

// close: what is sent before it can still be received, and then the receivers are told
START_TEST(chan_test10)

chan_int_t *c = chan_int_init(4);
int v;
CHECK(chan_int_tryrecv(c, &v) == CHAN_EMPTY);       // open and empty
CHECK(chan_int_trysend(c, 10) == CHAN_SUCCESS);
CHECK(chan_int_trysend(c, 20) == CHAN_SUCCESS);
CHECK(chan_int_close(c) == CHAN_SUCCESS);
CHECK(chan_int_close(c) == CHAN_CLOSED);            // already closed
CHECK(chan_int_trysend(c, 30) == CHAN_CLOSED);
CHECK(chan_int_send(c, 30) == CHAN_CLOSED);
CHECK(chan_int_tryrecv(c, &v) == CHAN_SUCCESS && v == 10);
CHECK(chan_int_recv(c, &v) == CHAN_SUCCESS && v == 20);
CHECK(chan_int_tryrecv(c, &v) == CHAN_CLOSED);      // drained and closed
CHECK(chan_int_recv(c, &v) == CHAN_CLOSED);
CHECK(chan_int_tryrecv(c, NULL) == CHAN_CLOSED);
chan_int_destroy(&c);

// a full channel which is closed: sends fail at once, and the contents can still be read
c = chan_int_init(2);
chan_int_trysend(c, 1);
chan_int_trysend(c, 2);
chan_int_close(c);
CHECK(chan_int_trysend(c, 3) == CHAN_CLOSED);       // closed is reported before full
CHECK(chan_int_tryrecv(c, &v) == CHAN_SUCCESS && v == 1);
CHECK(chan_int_trysend(c, 3) == CHAN_CLOSED);       // and there is room, but no sends
chan_int_destroy(&c);

// closing wakes a receiver which waits on an empty channel, and a sender which waits on a full one
c = chan_int_init(1);
pthread_t t;
CHECK(pthread_create(&t, NULL, chan_blocked_recv, c) == 0);
short_sleep(30);
CHECK(chan_int_close(c) == CHAN_SUCCESS);
void *rc;
CHECK(pthread_join(t, &rc) == 0);
CHECK((long) rc == CHAN_CLOSED);
chan_int_destroy(&c);

c = chan_int_init(1);
chan_int_trysend(c, 1);
CHECK(pthread_create(&t, NULL, chan_blocked_send, c) == 0);
short_sleep(30);
CHECK(chan_int_close(c) == CHAN_SUCCESS);
CHECK(pthread_join(t, &rc) == 0);
CHECK((long) rc == CHAN_CLOSED);
chan_int_destroy(&c);

// a blocked sender goes on when a receiver makes room
c = chan_int_init(1);
chan_int_trysend(c, 1);
CHECK(pthread_create(&t, NULL, chan_blocked_send, c) == 0);
short_sleep(30);
CHECK(chan_int_recv(c, &v) == CHAN_SUCCESS && v == 1);
CHECK(pthread_join(t, &rc) == 0);
CHECK((long) rc == CHAN_SUCCESS);
CHECK(chan_int_tryrecv(c, &v) == CHAN_SUCCESS && v == 1);
chan_int_destroy(&c);

END_TEST

// several senders and several receivers: nothing lost, nothing duplicated, and each sender's
// values arrive in the order they were sent
START_TEST(chan_test11)

int64_t caps[] = {1, 3, 8, 64};
int shapes[][2] = {{1, 1}, {3, 1}, {1, 3}, {4, 4}};   // producers, consumers
for (int ci = 0; ci < 4; ci++) {
  for (int si = 0; si < 4; si++) {
    int producers = shapes[si][0], consumers = shapes[si][1];
    int per = 5000;
    chan_int_t *c = chan_int_init(caps[ci]);
    pthread_t pt[8], ct[8];
    chan_producer_t pa[8];
    chan_consumer_t ca[8];
    for (int i = 0; i < consumers; i++) {
      ca[i].c = c;
      CHECK(pthread_create(&ct[i], NULL, chan_consumer, &ca[i]) == 0);
    }
    for (int i = 0; i < producers; i++) {
      pa[i] = (chan_producer_t) {c, per, i + 1, 0};
      CHECK(pthread_create(&pt[i], NULL, chan_producer, &pa[i]) == 0);
    }
    for (int i = 0; i < producers; i++) {
      pthread_join(pt[i], NULL);
      CHECK(pa[i].rc == CHAN_SUCCESS);
    }
    CHECK(chan_int_close(c) == CHAN_SUCCESS);
    long count = 0, sum = 0;
    for (int i = 0; i < consumers; i++) {
      pthread_join(ct[i], NULL);
      CHECK(ca[i].ordered);
      count += ca[i].count;
      sum += ca[i].sum;
    }
    long expected = 0;
    for (int id = 1; id <= producers; id++) {
      for (int i = 0; i < per; i++) {
        expected += id * 100000L + i;
      }
    }
    CHECK(count == (long) producers * per);
    CHECK(sum == expected);
    chan_int_destroy(&c);
  }
}

END_TEST

// select: errors, a case which is ready, none ready, cases which are finished
START_TEST(chan_test12)

select_int_t s[3];
int a_value = 0, b_value = 0, c_value = 0;
chan_int_t *a = chan_int_init(2);
chan_int_t *b = chan_int_init(2);
chan_int_t *c = chan_int_init(2);

// no cases, a negative number of cases, and cases which are NULL
CHECK(select_int_one(0, NULL) == SELECT_DONE);
CHECK(select_int_one(0, s) == SELECT_DONE);
CHECK(select_int_one(-1, s) == CHAN_ERROR);
CHECK(select_int_one(2, NULL) == CHAN_ERROR);

// nothing is ready: the answer is the number of cases
s[0] = (select_int_t) {a, SELECT_RECV, &a_value};
s[1] = (select_int_t) {b, SELECT_RECV, &b_value};
CHECK(select_int_one(2, s) == 2);

// one ready receive
chan_int_trysend(b, 42);
CHECK(select_int_one(2, s) == 1);
CHECK(b_value == 42);
CHECK(select_int_one(2, s) == 2);   // and it was taken

// a send which has room, and one which has not
int sent = 7;
s[0] = (select_int_t) {c, SELECT_SEND, &sent};
s[1] = (select_int_t) {a, SELECT_RECV, &a_value};
CHECK(select_int_one(2, s) == 0);
CHECK(chan_int_tryrecv(c, &c_value) == CHAN_SUCCESS && c_value == 7);
chan_int_trysend(c, 1);
chan_int_trysend(c, 2);             // c is full
CHECK(select_int_one(2, s) == 2);
CHECK(chan_int_tryrecv(c, &c_value) == CHAN_SUCCESS && c_value == 1);
CHECK(select_int_one(2, s) == 0);   // there is room again
chan_int_tryrecv(c, NULL);
chan_int_tryrecv(c, NULL);

// a case on a NULL channel is an error
s[0] = (select_int_t) {NULL, SELECT_RECV, &a_value};
s[1] = (select_int_t) {a, SELECT_RECV, &b_value};
int32_t seen_error = 0;
for (int i = 0; i < 40; i++) {      // the order of the cases is random: it is found sooner or later
  if (select_int_one(2, s) == CHAN_ERROR) {
    seen_error = 1;
    break;
  }
}
CHECK(seen_error);

// A case on a closed channel which is empty is finished: select marks it SELECT_OMIT.  The call
// which finds the channels closed reports that no case was ready (the number of cases), and the
// next call, which has only finished cases, reports SELECT_DONE.
chan_int_trysend(a, 5);
chan_int_close(a);
chan_int_close(b);
s[0] = (select_int_t) {a, SELECT_RECV, &a_value};
s[1] = (select_int_t) {b, SELECT_RECV, &b_value};
int32_t first = select_int_one(2, s);
CHECK(first == 0 && a_value == 5);            // the value which was in a
CHECK(select_int_one(2, s) == 2);             // a and b are found closed and empty
CHECK(s[0].select_type == SELECT_OMIT && s[1].select_type == SELECT_OMIT);
CHECK(select_int_one(2, s) == SELECT_DONE);
CHECK(select_int_one(2, s) == SELECT_DONE);   // and stays so

// a send to a closed channel is finished too
chan_int_t *d = chan_int_init(1);
chan_int_close(d);
s[0] = (select_int_t) {d, SELECT_SEND, &sent};
CHECK(select_int_one(1, s) == 1);
CHECK(s[0].select_type == SELECT_OMIT);
CHECK(select_int_one(1, s) == SELECT_DONE);

// a finished case is skipped, and the other cases go on
chan_int_t *e = chan_int_init(1);
chan_int_trysend(e, 99);
s[0] = (select_int_t) {d, SELECT_RECV, &a_value};
s[0].select_type = SELECT_OMIT;
s[1] = (select_int_t) {e, SELECT_RECV, &b_value};
CHECK(select_int_one(2, s) == 1 && b_value == 99);

// select_option_done finishes a case and closes its channel
chan_int_t *f = chan_int_init(1);
s[0] = (select_int_t) {f, SELECT_RECV, &a_value};
CHECK(select_int_option_done(1, s, 0) == 0);
CHECK(s[0].select_type == SELECT_OMIT);
CHECK(chan_int_trysend(f, 1) == CHAN_CLOSED);
CHECK(select_int_option_done(1, s, 0) == 0);   // closing a closed channel is fine

// select_option_done checks its arguments
CHECK(select_int_option_done(1, NULL, 0) == CHAN_ERROR);
CHECK(select_int_option_done(1, s, -1) == CHAN_ERROR);
CHECK(select_int_option_done(1, s, 1) == CHAN_ERROR);
CHECK(select_int_option_done(0, s, 0) == CHAN_ERROR);
s[0] = (select_int_t) {NULL, SELECT_RECV, &a_value};
CHECK(select_int_option_done(1, s, 0) == CHAN_ERROR);   // a NULL channel

// A SEND case needs a value to send.  The cases are checked first, so this is an error whether
// or not another case happens to be ready, and nothing is received.
chan_int_t *g = chan_int_init(1);
chan_int_t *h = chan_int_init(1);
chan_int_trysend(g, 5);
for (int i = 0; i < 40; i++) {
  s[0] = (select_int_t) {g, SELECT_RECV, &a_value};
  s[1] = (select_int_t) {h, SELECT_SEND, NULL};
  CHECK(select_int_one(2, s) == CHAN_ERROR);
}
CHECK(chan_int_tryrecv(g, &c_value) == CHAN_SUCCESS && c_value == 5);   // still there
CHECK(chan_int_tryrecv(h, &c_value) == CHAN_EMPTY);

// a RECV case may have no value pointer: the value is discarded
chan_int_trysend(g, 6);
s[0] = (select_int_t) {g, SELECT_RECV, NULL};
CHECK(select_int_one(1, s) == 0);
CHECK(chan_int_tryrecv(g, &c_value) == CHAN_EMPTY);

// the number of cases is limited
select_int_t many[SELECT_MAX_CASES + 1];
for (int i = 0; i <= SELECT_MAX_CASES; i++) {
  many[i] = (select_int_t) {g, SELECT_OMIT, NULL};
}
CHECK(select_int_one(SELECT_MAX_CASES, many) == SELECT_DONE);
CHECK(select_int_one(SELECT_MAX_CASES + 1, many) == CHAN_ERROR);
CHECK(select_int_one(1 << 30, many) == CHAN_ERROR);
// and the largest number works for a case which is ready, wherever it is
for (int i = 0; i < SELECT_MAX_CASES; i++) {
  many[i] = (select_int_t) {g, SELECT_RECV, &a_value};
}
chan_int_trysend(g, 77);
CHECK(select_int_one(SELECT_MAX_CASES, many) >= 0 && a_value == 77);
chan_int_destroy(&g);
chan_int_destroy(&h);

chan_int_destroy(&a);
chan_int_destroy(&b);
chan_int_destroy(&c);
chan_int_destroy(&d);
chan_int_destroy(&e);
chan_int_destroy(&f);

END_TEST

// select picks among ready cases at random, so that none of them can starve the others
START_TEST(chan_test13)

chan_int_t *ch[3];
select_int_t s[3];
int values[3];
for (int i = 0; i < 3; i++) {
  ch[i] = chan_int_init(1);
  s[i] = (select_int_t) {ch[i], SELECT_RECV, &values[i]};
}

// two ready cases, then three
long chosen2[2] = {0, 0};
for (int trial = 0; trial < 6000; trial++) {
  chan_int_trysend(ch[0], 0);
  chan_int_trysend(ch[1], 1);
  int32_t r = select_int_one(2, s);
  CHECK(r == 0 || r == 1);
  chosen2[r]++;
  chan_int_tryrecv(ch[0], NULL);
  chan_int_tryrecv(ch[1], NULL);
}
CHECK(chosen2[0] > 2500 && chosen2[0] < 3500);   // 3000 expected, with a spread of about 40

long chosen3[3] = {0, 0, 0};
for (int trial = 0; trial < 9000; trial++) {
  for (int i = 0; i < 3; i++) {
    chan_int_trysend(ch[i], i);
  }
  int32_t r = select_int_one(3, s);
  CHECK(r >= 0 && r < 3);
  chosen3[r]++;
  for (int i = 0; i < 3; i++) {
    chan_int_tryrecv(ch[i], NULL);
  }
}
for (int i = 0; i < 3; i++) {
  CHECK(chosen3[i] > 2500 && chosen3[i] < 3500);   // 3000 expected
}

for (int i = 0; i < 3; i++) {
  chan_int_destroy(&ch[i]);
}

END_TEST

typedef struct {
  chan_int_t *c;
  int n;
  int id;
} chan_closer_t;

// sends n values and then closes its own channel
static void *chan_send_then_close(void *p) {
  chan_closer_t *a = p;
  for (int i = 0; i < a->n; i++) {
    chan_int_send(a->c, a->id * 100000 + i);
  }
  chan_int_close(a->c);
  return NULL;
}

// several channels are read by one select loop which ends when they are all closed and empty
START_TEST(chan_test14)

#define FAN_IN 4
chan_int_t *ch[FAN_IN];
select_int_t s[FAN_IN];
int values[FAN_IN];
pthread_t t[FAN_IN];
chan_closer_t arg[FAN_IN];
int n = 3000;
for (int i = 0; i < FAN_IN; i++) {
  ch[i] = chan_int_init(i + 1);
  s[i] = (select_int_t) {ch[i], SELECT_RECV, &values[i]};
  arg[i] = (chan_closer_t) {ch[i], n, i + 1};
  CHECK(pthread_create(&t[i], NULL, chan_send_then_close, &arg[i]) == 0);
}

long count = 0, sum = 0;
int last[FAN_IN];
int ordered = 1;
for (int i = 0; i < FAN_IN; i++) {
  last[i] = -1;
}
while (true) {
  int32_t r = select_int_one(FAN_IN, s);
  if (r == SELECT_DONE) {
    break;
  }
  CHECK(r != CHAN_ERROR);
  if (r == FAN_IN) {
    sched_yield();
    continue;
  }
  int seq = values[r] % 100000;
  if (values[r] / 100000 != r + 1 || seq <= last[r]) {
    ordered = 0;
  }
  last[r] = seq;
  count++;
  sum += values[r];
}
for (int i = 0; i < FAN_IN; i++) {
  pthread_join(t[i], NULL);
}
long expected = 0;
for (int id = 1; id <= FAN_IN; id++) {
  for (int i = 0; i < n; i++) {
    expected += id * 100000L + i;
  }
}
CHECK(ordered);
CHECK(count == (long) FAN_IN * n);
CHECK(sum == expected);
for (int i = 0; i < FAN_IN; i++) {
  chan_int_destroy(&ch[i]);
}
#undef FAN_IN

END_TEST
