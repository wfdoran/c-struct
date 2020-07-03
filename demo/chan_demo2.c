#include <stdint.h>
#include <stdio.h>

#define data_t int64_t
#define prefix int64
#include <chan.h>
#undef prefix
#undef data_t

void produce(chan_int64_t *ch, int64_t n) {
  for (int64_t i = 1; i <= n; i++) {
    chan_int64_send(ch, i);
  }
  chan_int64_close(ch);
}

void square_em(chan_int64_t *in, chan_int64_t *out) {
  int64_t x;
  while (chan_int64_recv(in, &x) == CHAN_OPEN) {
    chan_int64_send(out, x * x);
  }
  chan_int64_close(out);
}

void sum_em(chan_int64_t *in, chan_int64_t *out) {
  int64_t total = 0;
  int64_t x;
  while (chan_int64_recv(in, &x) == CHAN_OPEN) {
    total += x;
  }
  chan_int64_send(out, total);
  chan_int64_close(out);
}

int main(void) {

  chan_int64_t *ch1 = chan_int64_init(10);
  chan_int64_t *ch2 = chan_int64_init(10);
  chan_int64_t *ch3 = chan_int64_init(10);
  

  #pragma omp parallel
  {
    #pragma omp sections
    {
      #pragma omp section
      produce(ch1, 1000);

      #pragma omp section
      square_em(ch1, ch2);

      #pragma omp section
      sum_em(ch2, ch3);
    }
  }

  int64_t total;
  chan_int64_recv(ch3, &total);
  printf("%ld\n", total);

  chan_int64_destroy(&ch1);
  chan_int64_destroy(&ch2);
  chan_int64_destroy(&ch3);
  return 0;
}
