#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#define data_t int64_t
#define prefix int64
#include <chan.h>
#undef prefix
#undef data_t

typedef struct {
  int64_t num_threads;
  int64_t my_thread;
  int64_t max_n;
  chan_int64_t *ch;
} produce_args_t;

void* produce(void *args) {
  produce_args_t *x = (produce_args_t *) args;

  for (int64_t i = x->my_thread; i <= x->max_n; i += x->num_threads) {
    chan_int64_send(x->ch, i);
  }
  return NULL;
}

typedef struct {
  int64_t num_threads;
  int64_t my_thread;
  int64_t max_n;
  chan_int64_t *in;
  chan_int64_t *out;
} consume_args_t;

void* consume(void *args) {
  consume_args_t *x = (consume_args_t *) args;

  int64_t sum = 0;
  for (int64_t i = x->my_thread; i <= x->max_n; i += x->num_threads) {
    int64_t val;
    chan_int64_recv(x->in, &val);

    if (val == INT64_C(-1)) {
      printf("%ld %ld\n", x->in->tail1, x->in->head0);
      fflush(stdout);
      // assert(0);
    }
    sum += val;
  }
  chan_int64_send(x->out, sum);
  return NULL;
}


int main(void) {
  int32_t capacity = 5;
  chan_int64_t *ch1 = chan_int64_init(capacity);
  chan_int64_t *ch2 = chan_int64_init(capacity);

  for (int32_t i = 0; i < capacity; i++) {
    ch1->data[i] = INT64_C(-1);
  }

  int64_t max_n = 1000000;

  int64_t num_producer_threads = 2;
  pthread_t p_threads[num_producer_threads];
  produce_args_t p_args[num_producer_threads];
  for (int64_t th = 0; th < num_producer_threads; th++) {
    p_args[th].num_threads = num_producer_threads;
    p_args[th].my_thread = th;
    p_args[th].max_n = max_n;
    p_args[th].ch = ch1;

    pthread_create(&p_threads[th], NULL, produce, (void*) &p_args[th]);
  }

  int64_t num_consumer_threads = 2;
  pthread_t c_threads[num_consumer_threads];
  consume_args_t c_args[num_consumer_threads];
  for (int64_t th = 0; th < num_consumer_threads; th++) {
    c_args[th].num_threads = num_consumer_threads;
    c_args[th].my_thread = th;
    c_args[th].max_n = max_n;
    c_args[th].in = ch1;
    c_args[th].out = ch2;

    pthread_create(&c_threads[th], NULL, consume, (void*) &c_args[th]);
  }

  int64_t total = 0;
  for (int64_t th = 0; th < num_consumer_threads; th++) {
    int64_t val = 0;
    chan_int64_recv(ch2, &val);
    total += val;
  }

  printf("%ld\n", total);
  printf("%ld\n", max_n * (max_n + 1) / 2);

  printf("%ld %ld %ld %ld\n", ch1->tail0, ch1->tail1, ch1->head0, ch1->head1);

  chan_int64_destroy(&ch1);
  chan_int64_destroy(&ch2);

  for (int64_t th = 0; th < num_producer_threads; th++) {
    pthread_join(p_threads[th], NULL);
  }

  for (int64_t th = 0; th < num_consumer_threads; th++) {
    pthread_join(c_threads[th], NULL);
  }
  
  return 0;
}
