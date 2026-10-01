#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <sched.h>
#include "cs_assert.h"
#include <time.h>
#include <sys/random.h>

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

/* size of a cache line; producer and consumer counters live on separate ones */
#define CHAN_CACHE_LINE (64)

/* Called in the loops that wait for an earlier sender or receiver to publish
   its slot.  That wait is normally very short, so spin first, but if the
   thread being waited for has been descheduled, give up the processor. */
static inline void chan_wait(int32_t *spins) {
  if (++*spins >= 64) {
    *spins = 0;
    sched_yield();
  }
}

/* Random numbers for select_prefix_one, which tries its cases in random
   order so that no ready case can starve the others.  This only needs
   fairness, not unpredictability, so each thread owns a small, fast
   splitmix64 generator:

     * thread safe: the state is _Thread_local, so there is nothing shared
       and no locking;
     * self-seeding: the first use in a thread seeds it from the operating
       system (getentropy).  If that fails, it falls back to mixing the
       clock, the address of the thread's own state, and a global counter,
       which still gives every thread a different stream.
*/
static _Thread_local uint64_t chan_rng_state;
static _Thread_local bool chan_rng_seeded = false;
static atomic_uint_fast64_t chan_rng_counter = 0;

static inline uint64_t chan_rng_next(void) {
  if (!chan_rng_seeded) {
    uint64_t seed;
    if (getentropy(&seed, sizeof(seed)) != 0) {
      struct timespec ts = {0, 0};
      timespec_get(&ts, TIME_UTC);
      seed = ((uint64_t) ts.tv_sec << 30) ^ (uint64_t) ts.tv_nsec;
      seed ^= (uint64_t) (uintptr_t) &chan_rng_state;
      seed += UINT64_C(0x9e3779b97f4a7c15) * (1 + (uint64_t) atomic_fetch_add(&chan_rng_counter, 1));
    }
    chan_rng_state = seed;
    chan_rng_seeded = true;
  }

  uint64_t z = (chan_rng_state += UINT64_C(0x9e3779b97f4a7c15));
  z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
  return z ^ (z >> 31);
}

/* Uniform value in [0, n) for n > 0, without modulo bias. */
static inline uint32_t chan_rng_below(uint32_t n) {
  const uint32_t threshold = (uint32_t) (-n) % n;   /* 2^32 mod n */
  uint32_t r;
  do {
    r = (uint32_t) (chan_rng_next() >> 32);
  } while (r < threshold);
  return r % n;
}
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
  data_t *data;
  int64_t capacity;           /* most values the channel holds at once */
  int64_t mask;               /* the slot array has mask + 1 entries, a power of two >= capacity */
  atomic_bool closed;

  /* written by the producers, on their own cache line */
  alignas(CHAN_CACHE_LINE) _Atomic int64_t head1;
  _Atomic int64_t head0;

  /* written by the consumers, on their own cache line */
  alignas(CHAN_CACHE_LINE) _Atomic int64_t tail1;
  _Atomic int64_t tail0;
} CHAN;

#define SELECT GLUE3(select_, prefix, _t)

typedef struct SELECT {
  CHAN *c;
  int select_type;
  data_t *value;
} SELECT;

/* chan_prefix_t *chan_prefix_init(int64_t capacity);

   Allocates a channel that buffers up to capacity values.  A capacity
   below 1 is rounded up to 1: unbuffered (rendezvous) channels are not
   provided.
*/
static inline CHAN *GLUE3(chan_, prefix, _init) (int64_t capacity) {
  if (capacity < 1) {
    capacity = 1;
  }

  /* slots are found with a mask, so round the slot array up to a power of two */
  int64_t slots = 1;
  while (slots < capacity) {
    if (slots > INT64_MAX / 2) {
      return NULL;
    }
    slots *= 2;
  }
  if ((uint64_t) slots > SIZE_MAX / sizeof(data_t)) {
    return NULL;
  }

  /* sizeof(CHAN) is a multiple of its alignment, as aligned_alloc requires */
  CHAN *c = aligned_alloc(alignof(CHAN), sizeof(CHAN));
  if (c == NULL) {
    return NULL;
  }
  c->capacity = capacity;
  c->mask = slots - 1;
  c->tail0 = 0;
  c->tail1 = 0;
  c->head0 = 0;
  c->head1 = 0;
  c->closed = false;

  c->data = malloc(slots * sizeof(data_t));
  if (c->data == NULL) {
    free(c);
    return NULL;
  }

  return c;
}

static inline void GLUE3(chan_, prefix, _destroy) (CHAN **c_ptr) {
  CHAN *c = *c_ptr;
  if (c == NULL) {
    return;
  }

  free(c->data);
  free(c);
  *c_ptr = NULL;
}

static inline int32_t GLUE3(chan_, prefix, _tryrecv) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  
  while (true) {
    int64_t tail1 = atomic_load(&c->tail1);
    int64_t head0 = atomic_load(&c->head0);
    ASSERT(tail1 <= head0);
    
    if (tail1 == head0) {
      if (!atomic_load(&c->closed)) {
        return CHAN_EMPTY;
      }
      /* A send that completed before the close may have landed after the
         emptiness check above, so look again before reporting CLOSED. */
      if (tail1 == atomic_load(&c->head0)) {
        return CHAN_CLOSED;
      }
      continue;
    }

    if (atomic_compare_exchange_weak(&c->tail1, &tail1, tail1 + 1)) {
      int64_t pos = tail1 & c->mask;
      if (value != NULL) {
        *value = c->data[pos];
      }
      int32_t spins = 0;
      while (true) {
        int64_t expect = tail1;
        if (atomic_compare_exchange_weak(&c->tail0, &expect, tail1 + 1)) {
          break;
        }
        chan_wait(&spins);
      }
      return CHAN_SUCCESS;
    }
  }
}

static inline int32_t GLUE3(chan_, prefix, _trysend) (CHAN *c, data_t value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  
  while (true) {
    if (c->closed) {
      return CHAN_CLOSED;
    }

    int64_t head1 = atomic_load(&c->head1);
    int64_t tail0 = atomic_load(&c->tail0);
    ASSERT(head1 <= tail0 + c->capacity);
    
    if (head1 == tail0 + c->capacity) {
      return CHAN_FULL;
    }

    if (atomic_compare_exchange_weak(&c->head1, &head1, head1 + 1)) {
      int64_t pos = head1 & c->mask;
      c->data[pos] = value;
      int32_t spins = 0;
      while (true) {
        int64_t expect = head1;
        if (atomic_compare_exchange_weak(&c->head0, &expect, head1 + 1)) {
          break;
        }
        chan_wait(&spins);
      }
      return CHAN_SUCCESS;
    }
  }
}

static inline int32_t GLUE3(chan_, prefix, _recv) (CHAN *c, data_t *value) {
  while (true) {
    int32_t rc = GLUE3(chan_, prefix, _tryrecv)(c, value);
    if (rc != CHAN_EMPTY) {
      return rc;
    }
    sched_yield();
  }
}

static inline int32_t GLUE3(chan_, prefix, _send) (CHAN *c, data_t value) {
  while (true) {
    int32_t rc = GLUE3(chan_, prefix, _trysend)(c, value);
    if (rc != CHAN_FULL) {
      return rc;
    }
    sched_yield();
  }
}

static inline int32_t GLUE3(chan_, prefix, _close) (CHAN *c) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  bool was_closed = atomic_exchange(&c->closed, true);
  return was_closed ? CHAN_CLOSED : CHAN_SUCCESS;
}

static inline int32_t GLUE3(select_, prefix, _one) (int32_t num_select, SELECT *s) {
  if (num_select < 0 || (num_select > 0 && s == NULL)) {
    return CHAN_ERROR;
  }
  if (num_select == 0) {
    return SELECT_DONE;
  }
  int32_t rc;
  bool all_omits = true;
  int32_t perm[num_select];
  for (int32_t i = 0; i < num_select; i++) {
    perm[i] = i;
  }
  for (int32_t i = 1; i < num_select; i++) {
    int32_t j = (int32_t) chan_rng_below((uint32_t) (i + 1));
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
	if (rc == CHAN_CLOSED) {
	  s[i].select_type = SELECT_OMIT;
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

static inline int32_t GLUE3(select_, prefix, _option_done)(SELECT *s, int32_t i) {
  s[i].select_type = SELECT_OMIT;
  int32_t rc = GLUE3(chan_, prefix, _close)(s[i].c);
  if (rc == CHAN_ERROR) {
    return CHAN_ERROR;
  }
  return 0;
}

#undef SELECT
#undef CHAN
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
