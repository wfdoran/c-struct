/* Interval arithmetic with directed rounding.  See doc/interval.md.

   An interval_t holds a lower and an upper bound and a valid flag.  Every
   function returns an interval which contains every exact result for every
   value in its argument intervals.  An operation which has no result for any
   value of its arguments (or is given an invalid interval) returns an invalid
   interval.

   Build with   gcc -O2 -frounding-math -I ../include prog.c -o prog -lm

   Every operation computes the upper bound under FE_UPWARD and the lower
   bound under FE_DOWNWARD.  An optimizer that treats the two computations as
   identical and shares them would make the result stop containing the exact
   answer, so each operation goes through iv_fence() to keep it in place, and
   -frounding-math is passed as an additional precaution.  Check once at
   startup that the rounding modes are honored with
   interval_rounding_is_honored().

   Every function is static inline.  Including this file turns on
   #pragma STDC FENV_ACCESS for clang in the rest of the translation unit.
 */

#ifndef INTERVAL_H
#define INTERVAL_H

#include <stdio.h>
#include <stdbool.h>
#include <fenv.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>

/* Math functions not provided yet:

     cbrt fmod log10 modf tan acos asin atan cosh sinh

   (https://www.w3schools.com/c/c_ref_math.php) */

#ifdef __clang__
#pragma STDC FENV_ACCESS ON
#endif

/* The math library functions are not guaranteed to honor the rounding mode
   or to be correctly rounded, so results from them are widened by one ulp. */
static inline double interval_ulp_up(double x) {
  return nextafter(x, INFINITY);
}

static inline double interval_ulp_down(double x) {
  return nextafter(x, -INFINITY);
}

/* Passing a value through a volatile object forces it to be computed at this
   point in the program.  The compiler may not move the operation above the
   fesetround() call before it or below the one after it, and may not share
   it with an identical operation done under another rounding mode.  (Neither
   -frounding-math nor FENV_ACCESS is enough for gcc to guarantee this.) */
static inline double iv_fence(double x) {
  volatile double v = x;
  return v;
}

typedef struct {
  double lo;
  double hi;
  bool valid;
} interval_t;

static inline interval_t interval_from_double(double x) {
  if (isnan(x)) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  interval_t rv = {.lo = nextafter(x, -INFINITY),
                   .hi = nextafter(x, INFINITY),
                   .valid = true};
  return rv;
}

static inline interval_t interval_from_int64(int64_t x) {
  interval_t rv;
  int save = fegetround();

  volatile int64_t vx = x;

  fesetround(FE_UPWARD);
  rv.hi = iv_fence((double) vx);

  fesetround(FE_DOWNWARD);
  rv.lo = iv_fence((double) vx);

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_add(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = iv_fence(iv_fence(a.hi) + iv_fence(b.hi));

  fesetround(FE_DOWNWARD);
  rv.lo = iv_fence(iv_fence(a.lo) + iv_fence(b.lo));

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_sub(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;

  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = iv_fence(iv_fence(a.hi) - iv_fence(b.lo));

  fesetround(FE_DOWNWARD);
  rv.lo = iv_fence(iv_fence(a.lo) - iv_fence(b.hi));

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_mul(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = iv_fence(iv_fence(a.lo) * iv_fence(b.lo));
  temp[1] = iv_fence(iv_fence(a.lo) * iv_fence(b.hi));
  temp[2] = iv_fence(iv_fence(a.hi) * iv_fence(b.lo));
  temp[3] = iv_fence(iv_fence(a.hi) * iv_fence(b.hi));
  rv.hi = fmax(fmax(fmax(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = iv_fence(iv_fence(a.lo) * iv_fence(b.lo));
  temp[1] = iv_fence(iv_fence(a.lo) * iv_fence(b.hi));
  temp[2] = iv_fence(iv_fence(a.hi) * iv_fence(b.lo));
  temp[3] = iv_fence(iv_fence(a.hi) * iv_fence(b.hi));
  rv.lo = fmin(fmin(fmin(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(save);

  /* fmin and fmax ignore NaN, which would silently drop a 0 * infinity term */
  if (isnan(temp[0]) || isnan(temp[1]) || isnan(temp[2]) || isnan(temp[3])) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_fma(interval_t a, interval_t b,
                                      interval_t c) {
  if (!a.valid || !b.valid || !c.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = iv_fence(fma(iv_fence(a.lo), iv_fence(b.lo), iv_fence(c.hi)));
  temp[1] = iv_fence(fma(iv_fence(a.lo), iv_fence(b.hi), iv_fence(c.hi)));
  temp[2] = iv_fence(fma(iv_fence(a.hi), iv_fence(b.lo), iv_fence(c.hi)));
  temp[3] = iv_fence(fma(iv_fence(a.hi), iv_fence(b.hi), iv_fence(c.hi)));
  rv.hi = fmax(fmax(fmax(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = iv_fence(fma(iv_fence(a.lo), iv_fence(b.lo), iv_fence(c.lo)));
  temp[1] = iv_fence(fma(iv_fence(a.lo), iv_fence(b.hi), iv_fence(c.lo)));
  temp[2] = iv_fence(fma(iv_fence(a.hi), iv_fence(b.lo), iv_fence(c.lo)));
  temp[3] = iv_fence(fma(iv_fence(a.hi), iv_fence(b.hi), iv_fence(c.lo)));
  rv.lo = fmin(fmin(fmin(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(save);

  /* fmin and fmax ignore NaN, which would silently drop a 0 * infinity term */
  if (isnan(temp[0]) || isnan(temp[1]) || isnan(temp[2]) || isnan(temp[3])) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_div(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  if (b.lo <= 0.0 && b.hi >= 0.0) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = iv_fence(iv_fence(a.lo) / iv_fence(b.lo));
  temp[1] = iv_fence(iv_fence(a.lo) / iv_fence(b.hi));
  temp[2] = iv_fence(iv_fence(a.hi) / iv_fence(b.lo));
  temp[3] = iv_fence(iv_fence(a.hi) / iv_fence(b.hi));
  rv.hi = fmax(fmax(fmax(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = iv_fence(iv_fence(a.lo) / iv_fence(b.lo));
  temp[1] = iv_fence(iv_fence(a.lo) / iv_fence(b.hi));
  temp[2] = iv_fence(iv_fence(a.hi) / iv_fence(b.lo));
  temp[3] = iv_fence(iv_fence(a.hi) / iv_fence(b.hi));
  rv.lo = fmin(fmin(fmin(temp[0], temp[1]), temp[2]), temp[3]);

  fesetround(save);

  /* fmin and fmax ignore NaN, which would silently drop a 0 * infinity term */
  if (isnan(temp[0]) || isnan(temp[1]) || isnan(temp[2]) || isnan(temp[3])) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_fmax(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = fmax(a.hi, b.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = fmax(a.lo, b.lo);
  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_fmin(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  if (a.hi < b.lo) {
    return a;
  }

  if (b.hi < a.lo) {
    return b;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = fmin(a.hi, b.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = fmin(a.lo, b.lo);
  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_exp(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = interval_ulp_up(exp(a.hi));

  fesetround(FE_DOWNWARD);
  rv.lo = fmax(interval_ulp_down(exp(a.lo)),
               0.0); /* exp is positive: do not widen below 0 */

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_erf(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = fmin(interval_ulp_up(erf(a.hi)), 1.0);

  fesetround(FE_DOWNWARD);
  rv.lo = fmax(interval_ulp_down(erf(a.lo)), -1.0);

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_sqrt(interval_t a) {
  /* the domain is [0, inf): use the part of the interval inside it, and fail
   * only if none of it is */
  if (!a.valid || !(a.hi >= 0.0)) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = iv_fence(sqrt(iv_fence(a.hi)));

  fesetround(FE_DOWNWARD);
  double lo = a.lo > 0 ? a.lo : 0.0;
  rv.lo = iv_fence(sqrt(iv_fence(lo)));

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_floor(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  interval_t rv = {.hi = floor(a.hi), .lo = floor(a.lo), .valid = true};
  return rv;
}

static inline interval_t interval_ceil(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  interval_t rv = {.hi = ceil(a.hi), .lo = ceil(a.lo), .valid = true};
  return rv;
}

static inline interval_t interval_log(interval_t a) {
  /* the domain is (0, inf): use the part of the interval inside it, and fail
   * only if none of it is */
  if (!a.valid || !(a.hi > 0.0)) {
    interval_t bad = {.lo = 0.0, .hi = 0.0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = interval_ulp_up(log(a.hi));

  fesetround(FE_DOWNWARD);
  double lo =
      a.lo > 0 ? a.lo : 0.0; /* log(0) is -inf: the lower bound is unbounded */
  rv.lo = interval_ulp_down(log(lo));

  fesetround(save);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_neg(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv = {.lo = -a.hi, .hi = -a.lo, .valid = true};
  return rv;
}

static inline interval_t interval_fabs(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  if (a.lo >= 0.0) {
    return a;
  }

  if (a.hi <= 0.0) {
    return interval_neg(a);
  }

  int save = fegetround();

  fesetround(FE_UPWARD);
  interval_t rv = {.lo = 0.0, .hi = fmax(fabs(a.lo), a.hi), .valid = true};

  fesetround(save);
  return rv;
}

#define data_t double
#define prefix ival
#include "pqueue.h"
#undef prefix
#undef data_t

static inline double interval_get_key(interval_t a) {
  return fmax(fabs(a.lo), fabs(a.hi));
}

/* pqueue pops the largest key first; this reverses the order. */
static inline int interval_key_min_first(double *a, double *b) {
  return comp_double(b, a);
}

static inline void interval_print(interval_t a) {
  if (!a.valid) {
    printf("[invalid]\n");
  } else {
    printf("[%.20f, %.20f]\n", a.lo, a.hi);
  }
}

static inline interval_t interval_add_many(int n, interval_t *a) {
  if (n <= 0 || a == NULL) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  for (int i = 0; i < n; i++) {
    if (!a[i].valid) {
      interval_t bad = {.lo = 0, .hi = 0, .valid = false};
      return bad;
    }
  }

  if (n == 1) {
    return a[0];
  }

  /* Repeatedly add the two intervals of smallest magnitude.  A priority queue
     is used rather than a tree because several intervals can have the same key.
   */
  interval_t bad = {.lo = 0, .hi = 0, .valid = false};
  interval_t rv = bad;

  pqueue_ival_t *q = pqueue_ival_init();
  interval_t *temp = malloc((n - 1) * sizeof(interval_t));
  if (q == NULL || temp == NULL ||
      pqueue_ival_set_comp(q, interval_key_min_first) != 0) {
    goto done;
  }

  for (int i = 0; i < n; i++) {
    if (pqueue_ival_push(q, interval_get_key(a[i]), &a[i]) != 0) {
      goto done;
    }
  }

  for (int i = 0; i < n - 1; i++) {
    pqkv_ival_t x = pqueue_ival_pop(q);
    pqkv_ival_t y = pqueue_ival_pop(q);
    if (!x.found || !y.found) {
      goto done;
    }

    temp[i] = interval_add(*(interval_t *) x.value, *(interval_t *) y.value);
    if (pqueue_ival_push(q, interval_get_key(temp[i]), &temp[i]) != 0) {
      goto done;
    }
  }
  rv = temp[n - 2];

done:
  pqueue_ival_destroy(&q);
  free(temp);
  return rv;
}

static inline interval_t interval_pow_uint(interval_t a, uint32_t e) {
  interval_t rv = interval_from_double(1.0);

  while (e > 0) {
    if ((e & 1) == 1) {
      rv = interval_mul(rv, a);
    }
    e >>= 1;
    if (e > 0) {
      a = interval_mul(a, a);
    }
  }

  return rv;
}

/* A negative exponent is the reciprocal of the positive power, which is
   invalid if that power contains zero (interval_div). */
static inline interval_t interval_pow_int(interval_t a, int32_t e) {
  if (e >= 0) {
    return interval_pow_uint(a, (uint32_t) e);
  }
  interval_t one = {.lo = 1.0, .hi = 1.0, .valid = true};
  return interval_div(one, interval_pow_uint(a, UINT32_C(0) - (uint32_t) e));
}

static inline interval_t interval_pow_dbl(interval_t a, interval_t b) {
  interval_t x1 = interval_log(a);
  interval_t x2 = interval_mul(x1, b);
  return interval_exp(x2);
}

#define interval_pow(x, e)                                                     \
  _Generic((e), int                                                            \
           : interval_pow_int, uint32_t                                        \
           : interval_pow_uint, interval_t                                     \
           : interval_pow_dbl)(x, e)

static inline interval_t interval_sin(interval_t a) {
  // sin(k * 2pi + 0 * pi/2) = 0
  // sin(k * 2pi + 1 * pi/2) = 1
  // sin(k * 2pi + 2 * pi/2) = 0
  // sin(k * 2pi + 3 * pi/2) = -1

  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;

  int save = fegetround();

  /* start with the min and max of values from a. */
  fesetround(FE_UPWARD);
  rv.hi = fmax(sin(a.lo), sin(a.hi));

  double mult_hi = iv_fence(iv_fence(a.hi) / M_PI_2);

  fesetround(FE_DOWNWARD);
  rv.lo = fmin(sin(a.lo), sin(a.hi));

  double mult_lo = iv_fence(iv_fence(a.lo) / M_PI_2);

  /* M_PI_2 is pi/2 rounded to a double (relative error 0.18 DBL_EPSILON), which
     makes every quotient too large by that fraction of itself, so a lower end
     just before an extremum can be judged to lie after it.  That is less than
     one unit in the last place, so step both quotients outward by one. */
  mult_lo = nextafter(mult_lo, -INFINITY);
  mult_hi = nextafter(mult_hi, INFINITY);

  /* The interval contains an extremum if it spans a point c + 4k in units of
     pi/2 (c = 1 and 3 for the sin maximum and minimum).  That is the case when
     floor((x - c) / 4) differs at the two ends; floor, not trunc, so that this
     also holds for negative arguments. */
  if (floor((mult_lo - 1.0) / 4.0) != floor((mult_hi - 1.0) / 4.0)) {
    rv.hi = 1.0;
  }

  if (floor((mult_lo - 3.0) / 4.0) != floor((mult_hi - 3.0) / 4.0)) {
    rv.lo = -1.0;
  }

  fesetround(save);
  rv.hi = fmin(interval_ulp_up(rv.hi), 1.0);
  rv.lo = fmax(interval_ulp_down(rv.lo), -1.0);
  rv.valid = true;

  return rv;
}

static inline interval_t interval_cos(interval_t a) {
  // cos(k * 2pi + 0 * pi/2) = 1
  // cos(k * 2pi + 1 * pi/2) = 0
  // cos(k * 2pi + 2 * pi/2) = -1
  // cos(k * 2pi + 3 * pi/2) = 0

  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;

  int save = fegetround();

  /* start with the min and max of values from a. */
  fesetround(FE_UPWARD);
  rv.hi = fmax(cos(a.lo), cos(a.hi));

  double mult_hi = iv_fence(iv_fence(a.hi) / M_PI_2);

  fesetround(FE_DOWNWARD);
  rv.lo = fmin(cos(a.lo), cos(a.hi));

  double mult_lo = iv_fence(iv_fence(a.lo) / M_PI_2);

  /* M_PI_2 is pi/2 rounded to a double (relative error 0.18 DBL_EPSILON), which
     makes every quotient too large by that fraction of itself, so a lower end
     just before an extremum can be judged to lie after it.  That is less than
     one unit in the last place, so step both quotients outward by one. */
  mult_lo = nextafter(mult_lo, -INFINITY);
  mult_hi = nextafter(mult_hi, INFINITY);

  /* Same test as in interval_sin, with c = 0 and 2 for the cos maximum and
     minimum. */
  if (floor((mult_lo - 0.0) / 4.0) != floor((mult_hi - 0.0) / 4.0)) {
    rv.hi = 1.0;
  }

  if (floor((mult_lo - 2.0) / 4.0) != floor((mult_hi - 2.0) / 4.0)) {
    rv.lo = -1.0;
  }

  fesetround(save);
  rv.hi = fmin(interval_ulp_up(rv.hi), 1.0);
  rv.lo = fmax(interval_ulp_down(rv.lo), -1.0);
  rv.valid = true;

  return rv;
}

/* Returns false if the compiler ignored the rounding mode changes: for
   inputs whose product is not exactly representable, the lower bound of
   [x,x]*[y,y] must be strictly below the upper bound.  Call it once at
   startup (the demos do) and refuse to run if it fails. */
static inline bool interval_rounding_is_honored(void) {
  volatile double x = 1.1;
  volatile double y = 1.3;
  interval_t a = {.lo = x, .hi = x, .valid = true};
  interval_t b = {.lo = y, .hi = y, .valid = true};
  interval_t p = interval_mul(a, b);
  return p.lo < p.hi;
}

#endif
