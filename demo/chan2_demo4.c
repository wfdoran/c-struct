#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define data_t int
#define prefix int
#include <chan2.h>
#undef prefix
#undef data_t

#define data_t float
#define prefix float
#include <chan2.h>
#undef prefix
#undef data_t


void produce(chan_int_t *ch, int n) {
  for (int i = 1; i <= n; i++) {
    chan_int_send(ch, i);
  }
  chan_int_close(ch);
}

void consume(chan_int_t *in, chan_float_t *out) {
  float total = 0.0;
  int count = 0;
  int x;
  while (chan_int_recv(in, &x) == CHAN_SUCCESS) {
    total += sqrt((float) x);
    count++;
  }

  printf("%s: %6d %8.4f\n", __func__, count, total);
  chan_float_send(out, total);
}

int main(void) {
  srand48(time(NULL));

  int capacity = 10;
  chan_int_t *ch1 = chan_int_init(capacity);
  
  chan_float_t *ch2 = chan_float_init(0);

  int nth = 6;
  int n = 1000;

  omp_set_dynamic(0);
#pragma omp parallel sections num_threads(nth + 2)
  {
     #pragma omp section
     produce(ch1, n);

     #pragma omp section
     consume(ch1, ch2);
     #pragma omp section
     consume(ch1, ch2);
     #pragma omp section
     consume(ch1, ch2);
     #pragma omp section
     consume(ch1, ch2);
     #pragma omp section
     consume(ch1, ch2);
     #pragma omp section
     consume(ch1, ch2);
     
     #pragma omp section
     {
       float total = 0.0;
       for (int th = 0; th < nth; th++) {
	 float v;
	 chan_float_recv(ch2, &v);
	 total += v;
	 
       }

       
       printf(" total = %20.8f\n", total); 
       printf("approx = %20.8f\n", (2.0 * n / 3.0) * sqrt(1.5 + n)); fflush(stdout);
     }
  }

  chan_int_destroy(&ch1);
  chan_float_destroy(&ch2);
  return 0;
}
  
