#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sched.h>
#include <assert.h>

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
#define SELECT_DONE (-2)

#define SELECT_SEND (0)
#define SELECT_RECV (1)
#define SELECT_OMIT (2)
#endif



#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define CHAN GLUE3(chan_, prefix, _t)

/* 
     assert tail0 <= tail1 <= head0 <= head1

     tail0   been read, ready for overwrite
     tail1   read position
     head0   been written, ready for reading
     head1   write position
*/ 

typedef struct CHAN {
  data_t * volatile data;
  int64_t capacity;
  _Atomic int64_t tail0;           
  _Atomic int64_t tail1;     
  _Atomic int64_t head0;
  _Atomic int64_t head1;
  atomic_bool closed;
} CHAN;

#define SELECT GLUE3(select_, prefix, _t)

typedef struct SELECT {
  CHAN *c;
  int select_type;
  data_t *value;
} SELECT;

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
  *c_ptr = NULL;
}

int32_t GLUE3(chan_, prefix, _tryrecv) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  
  while (true) {
    int64_t tail1 = atomic_load(&c->tail1);
    int64_t head0 = atomic_load(&c->head0);
    assert(tail1 <= head0);
    
    if (tail1 == head0) {
      return c->closed ? CHAN_CLOSED : CHAN_EMPTY;
    }

    if (atomic_compare_exchange_strong(&c->tail1, &tail1, tail1 + 1)) {
      int64_t pos = tail1 % c->capacity;
      *value = c->data[pos];
      while (true) {
        int64_t expect = tail1;
        if (atomic_compare_exchange_strong(&c->tail0, &expect, tail1 + 1)) {
          break;
        }
      }
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

    int64_t head1 = atomic_load(&c->head1);
    int64_t tail0 = atomic_load(&c->tail0);
    assert(head1 <= tail0 + c->capacity);
    
    if (head1 == tail0 + c->capacity) {
      return CHAN_FULL;
    }

    if (atomic_compare_exchange_strong(&c->head1, &head1, head1 + 1)) {
      int64_t pos = head1 % c->capacity;
      c->data[pos] = value;
      while (true) {
        int64_t expect = head1;
        if (atomic_compare_exchange_strong(&c->head0, &expect, head1 + 1)) {
          break;
        }
      }
      return CHAN_SUCCESS;
    }
  }
}

int32_t GLUE3(chan_, prefix, _recv) (CHAN *c, data_t *value) {
  while (true) {
    int32_t rc = GLUE3(chan_, prefix, _tryrecv)(c, value);
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

int32_t GLUE3(select_, prefix, _one) (int32_t num_select, SELECT *s) {
  if (num_select == 0) {
    return 0;
  }
  int32_t rc;
  bool all_omits = true;
  int32_t perm[num_select];
  for (int32_t i = 0; i < num_select; i++) {
    perm[i] = i;
  }
  for (int32_t i = 1; i < num_select; i++) {
    int32_t j = lrand48() % (i + 1);
    int32_t temp = perm[i];
    perm[i] = perm[j];
    perm[j] = temp;
  }
  for (int32_t i_idx = 0; i_idx < num_select; i_idx++) {
    int32_t i = perm[i_idx];
    switch(s[i].select_type) {
      case SELECT_SEND:
        all_omits = false;
        rc = GLUE3(chan_, prefix, _trysend)(s[i].c, *s[i].value);
        if (rc == CHAN_SUCCESS) {
          return i;
        }
        if (rc == CHAN_ERROR) {
          return CHAN_ERROR;
        }
        break;  // CHAN_FULL or CHAN_CLOSED
      case SELECT_RECV:
        all_omits = false;
        rc = GLUE3(chan_, prefix, _tryrecv)(s[i].c, s[i].value);
        if (rc == CHAN_SUCCESS) {
          return i;
        }
        if (rc == CHAN_ERROR) {
          return CHAN_ERROR;
        }
        if (rc == CHAN_CLOSED) {
          s[i].select_type = SELECT_OMIT;
        }
        break;  // CHAN_EMPTY or CHAN_CLOSED

      default:
        break;
    }
  }
  if (all_omits) {
    return SELECT_DONE;
  }
  return num_select;
}

int32_t GLUE3(select_, prefix, _option_done)(SELECT *s, int32_t i) {
  s[i].select_type = SELECT_OMIT;
  int32_t rc = GLUE3(chan_, prefix, _close)(s[i].c);
  if (rc == CHAN_ERROR) {
    return CHAN_ERROR;
  }
  return 0;
}

#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
