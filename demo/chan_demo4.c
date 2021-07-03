#include <stdio.h>
#include <math.h>

#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

#define data_t float
#define prefix float
#include <chan.h>
#undef prefix
#undef data_t


void produce(chan_int_t *ch) {
  for (int i = 0; i < 100; i++) {
    chan_int_send(ch, i);
  }
  chan_int_close(ch);
}

void consume(chan_int_t *in, chan_float_t *out) {
  float total = 0.0;
  int x;
  while (chan_int_recv(in, &x) == CHAN_SUCCESS) {
    total += sqrt((float) x);
  }

  chan_float_send(out, total);
}

int main(void) {
  chan_int_t *ch1 = chan_int_init(10);
  chan_float_t *ch2 = chan_float_init(0);

  int nth = 6;

  #pragma omp parallel sections
  {
     #pragma omp section
     produce(ch1);

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

       printf("total = %20.8f\n", total); fflush(stdout);
     }
  }

  chan_int_destroy(&ch1);
  chan_float_destroy(&ch2);
  return 0;
}
  
