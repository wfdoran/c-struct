#include <stdint.h>
#include <stdio.h>

#define data_t int64_t
#define prefix int64
#include <chan2.h>
#undef prefix
#undef data_t

void produce_and_sum(chan_int64_t *ch1, chan_int64_t *ch2, chan_int64_t *ch3, int64_t n) {
  int64_t idx = 1;
  int64_t total = 0;
  int64_t x;

  while(true) {
    if (idx <= n) {
      if (chan_int64_trysend(ch1, idx) == CHAN_SUCCESS) {
	idx++;
	if (idx > n) {
	  chan_int64_close(ch1);
	}
      }
    }

    int32_t rc = chan_int64_tryrecv(ch2, &x);
    if (rc == CHAN_SUCCESS) {
      total += x;
    }
    if (rc == CHAN_CLOSED) {
      chan_int64_send(ch3, total);
      chan_int64_close(ch3);
      break;
    }
  }
}

void square_em(chan_int64_t *in, chan_int64_t *out) {
  int64_t x;
  while (chan_int64_recv(in, &x) == CHAN_SUCCESS) {
    chan_int64_send(out, x * x);
  }
  chan_int64_close(out);
}

int main(void) {

  int32_t capacity = 5;
  chan_int64_t *ch1 = chan_int64_init(capacity);
  chan_int64_t *ch2 = chan_int64_init(capacity);
  chan_int64_t *ch3 = chan_int64_init(capacity);
  
  int64_t n = 1000;
  #pragma omp parallel
  {
    #pragma omp sections
    {
      #pragma omp section
      produce_and_sum(ch1, ch2, ch3, n);

      #pragma omp section
      square_em(ch1, ch2);
    }
  }

  int64_t total;
  chan_int64_recv(ch3, &total);
  printf("%ld\n", total);
  printf("%ld\n", n * (n - 1) * (2 * n - 1) / 6);

  chan_int64_destroy(&ch1);
  chan_int64_destroy(&ch2);
  chan_int64_destroy(&ch3);
  return 0;
}
