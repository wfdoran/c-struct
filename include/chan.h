#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#ifndef CHAN_OPEN
#define CHAN_OPEN (0)
#endif

#ifndef CHAN_CLOSED
#define CHAN_CLOSED (1)
#endif



#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define CHAN GLUE3(chan_, prefix, _t)

typedef struct CHAN {
    data_t *data;
    int64_t capacity;
    int64_t occupancy;
    int64_t read_pos;
    int64_t write_pos;
    bool closed;
    pthread_rwlock_t rwlock;
} CHAN;

/* 
    chan_prefix_t *chan_prefix_init(int64_t capacity);
    void chan_prefix_destroy(chan_prefix_t **c);
    int32_t chan_prefix_send(chan_prefix_t *c, data_t value);
    int32_t chan_prefix_recv(chan_prefix_t *c, data_t *value);
    int32_t chan_close(chan_prefix_t *c);
*/
    
  
CHAN *GLUE3(chan_, prefix, _init) (int64_t capacity) {
  CHAN *c = malloc(sizeof(CHAN));
  if (c == NULL) {
    return NULL;
  }
  c->capacity = capacity > 0 ? capacity : 1;
  c->occupancy = 0;
  c->read_pos = 0;
  c->write_pos = 0;
  c->closed = false;

  c->data = malloc(c->capacity * sizeof(data_t));
  if (c->data == NULL) {
    return NULL;
  }

  pthread_rwlock_init(&(c->rwlock), NULL);

  return c;
}

void GLUE3(chan_, prefix, _destroy) (CHAN **c_ptr) {
  CHAN *c = *c_ptr;
  if (c == NULL) {
    return;
  }

  pthread_rwlock_wrlock(&(c->rwlock));
  free(c->data);
  pthread_rwlock_destroy(&(c->rwlock));
  free(c);
  c_ptr = NULL;
}

int32_t GLUE3(chan_, prefix, _send) (CHAN *c, data_t value) {
  if (c == NULL) {
    return -1;
  }

  struct timespec sleep = {.tv_sec = 0, .tv_nsec = 500L};
  int64_t max_sleep_nsec = INT64_C(1000000); // one millisec
  while (true) {
    pthread_rwlock_wrlock(&(c->rwlock));

    if (c->closed) {
      pthread_rwlock_unlock(&(c->rwlock));
      return -2;
    }

    if (c->occupancy < c->capacity) {
      c->data[c->write_pos] = value;
      c->write_pos = (c->write_pos + 1) % c->capacity;
      c->occupancy++;
      pthread_rwlock_unlock(&(c->rwlock));
      return 0;
    }
    pthread_rwlock_unlock(&(c->rwlock));

    sleep.tv_nsec *= 2;
    if (sleep.tv_nsec > max_sleep_nsec) {
      sleep.tv_nsec = max_sleep_nsec;
    }
    nanosleep(&sleep, NULL);
  }
}

int32_t GLUE3(chan_, prefix, _recv) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return -1;
  }

  struct timespec sleep = {.tv_sec = 0, .tv_nsec = 500L};
  int64_t max_sleep_nsec = INT64_C(1000000); // one millisec
  while (true) {
    pthread_rwlock_wrlock(&(c->rwlock));

    if (c->occupancy > 0) {
      *value = c->data[c->read_pos];
      c->read_pos = (c->read_pos + 1) % c->capacity;
      c->occupancy--;
      pthread_rwlock_unlock(&(c->rwlock));
      return CHAN_OPEN;
    }

    if (c->closed) {
      pthread_rwlock_unlock(&(c->rwlock));
      return CHAN_CLOSED;
    }

    pthread_rwlock_unlock(&(c->rwlock));

    sleep.tv_nsec *= 2;
    if (sleep.tv_nsec > max_sleep_nsec) {
      sleep.tv_nsec = max_sleep_nsec;
    }
    nanosleep(&sleep, NULL);
  }
}

int32_t GLUE3(chan_, prefix, _close) (CHAN *c) {
  if (c == NULL) {
    return -1;
  }
  pthread_rwlock_wrlock(&(c->rwlock));
  int32_t rc = c->closed ? -2 : 0;
  c->closed = true;
  pthread_rwlock_unlock(&(c->rwlock));
  return rc;
}

#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
