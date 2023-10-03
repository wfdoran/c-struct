#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sched.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif


#ifndef CHAN_CONSTS
#define CHAN_CONSTS

#define CHAN_SUCCESS (0)
#define CHAN_CLOSED (1)
#define CHAN_FULL (2)
#define CHAN_EMPTY (3)

#define CHAN_ERROR (-1)
#endif



#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define CHAN GLUE3(chan_, prefix, _t)

/* 
     assert tail0 <= tail1 <= head0 <= head1


*/ 

typedef struct CHAN {
  data_t *data;
  int64_t capacity;
  _Atomic int64_t tail0;           
  _Atomic int64_t tail1;     
  _Atomic int64_t head0;
  _Atomic int64_t head1;
  atomic_bool closed;
} CHAN;
      
CHAN *GLUE3(chan_, prefix, _init) (int64_t capacity) {
  CHAN *c = malloc(sizeof(CHAN));
  if (c == NULL) {
    return NULL;
  }
  c->capacity = capacity > 0 ? capacity : 1;
  c->tail0 = 0;
  c->tail1 = 0;
  c->head0 = 0;
  c->head1 = 0;
  c->closed = false;

  c->data = malloc(c->capacity * sizeof(data_t));
  if (c->data == NULL) {
    return NULL;
  }

  return c;
}

void GLUE3(chan_, prefix, _destroy) (CHAN **c_ptr) {
  CHAN *c = *c_ptr;
  if (c == NULL) {
    return;
  }

  free(c->data);
  free(c);
  c_ptr = NULL;
}

int32_t GLUE3(chan_, prefix, _tryread) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  while (true) {
    int64_t tail1 = atomic_load(&c->tail1);
    int64_t head0 = atomic_load(&c->head0);
    if (tail1 == head0) {
      return c->closed ? CHAN_CLOSED : CHAN_EMPTY;
    }

    if (atomic_compare_exchange_strong(&c->tail1, &tail1, tail1+1)) {
      int64_t pos = tail1 % c->capacity;
      *value = c->data[pos];
      while (!atomic_compare_exchange_strong(&c->tail0, &tail1, tail1+1));
      return CHAN_SUCCESS;
    }
  }
}

int32_t GLUE3(chan_, prefix, _trysend) (CHAN *c, data_t value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  while (true) {
    if (c->closed) {
      return CHAN_CLOSED;
    }

    int64_t tail0 = atomic_load(&c->tail0);
    int64_t head1 = atomic_load(&c->head1);

    if (head1 >= tail0 + c->capacity) {
      return CHAN_FULL;
    }

    if (atomic_compare_exchange_strong(&c->head1, &head1, head1+1)) {
      int64_t pos = head1 % c->capacity;
      c->data[pos] = value;
      while (!atomic_compare_exchange_strong(&c->head0, &head1, head1 + 1));
      return CHAN_SUCCESS;
    }
  }
}

int32_t GLUE3(chan_, prefix, _read) (CHAN *c, data_t *value) {
  while (true) {
    int32_t rc = GLUE3(chan_, prefix, _tryread)(c, value);
    if (rc != CHAN_EMPTY) {
      return rc;
    }
    sched_yield();
  }
}

int32_t GLUE3(chan_, prefix, _send) (CHAN *c, data_t value) {
  while (true) {
    int32_t rc = GLUE3(chan_, prefix, _trysend)(c, value);
    if (rc != CHAN_FULL) {
      return rc;
    }
    sched_yield();
  }
}

int32_t GLUE3(chan_, prefix, _close) (CHAN *c) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  atomic_store(&c->closed, true);
  return CHAN_CLOSED;
}


#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
