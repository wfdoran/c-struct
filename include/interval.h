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

#include "cs_private.h"

/* M_PI_2 is POSIX, not C11: define it when the system does not (the same
   double as the system value). */
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

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

/* The result of an operation which has no result, or is given an invalid
   interval. */
static inline interval_t interval_invalid(void) {
  interval_t rv = {.lo = 0, .hi = 0, .valid = false};
  return rv;
}

static inline interval_t interval_from_double(double x) {
  if (isnan(x)) {
    return interval_invalid();
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
    return interval_invalid();
  }

  interval_t rv;
  int save = fegetround();

  /* rounding down x + y is the same as -(rounding up (-x) + (-y)), so both
     bounds are computed under FE_UPWARD with a single mode switch */
  fesetround(FE_UPWARD);
  rv.hi = iv_fence(iv_fence(a.hi) + iv_fence(b.hi));
  rv.lo = -iv_fence(iv_fence(-a.lo) + iv_fence(-b.lo));

  fesetround(save);

  /* infinity - infinity: as in interval_mul, a NaN bound is not a valid
     interval */
  if (isnan(rv.lo) || isnan(rv.hi)) {
    return interval_invalid();
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_sub(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    return interval_invalid();
  }

  interval_t rv;

  int save = fegetround();

  /* rounding down x - y is the same as -(rounding up y - x), see
   * interval_add */
  fesetround(FE_UPWARD);
  rv.hi = iv_fence(iv_fence(a.hi) - iv_fence(b.lo));
  rv.lo = -iv_fence(iv_fence(b.hi) - iv_fence(a.lo));

  fesetround(save);

  /* infinity - infinity: as in interval_mul, a NaN bound is not a valid
     interval */
  if (isnan(rv.lo) || isnan(rv.hi)) {
    return interval_invalid();
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_mul(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    return interval_invalid();
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
    return interval_invalid();
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_fma(interval_t a, interval_t b,
                                      interval_t c) {
  if (!a.valid || !b.valid || !c.valid) {
    return interval_invalid();
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
    return interval_invalid();
  }

  rv.valid = true;
  return rv;
}

/* The quotient when b contains 0.  Only the part of b which is not 0 has a
   quotient: (0, b.hi] and [b.lo, 0), and near 0 the quotients are unbounded.
   The result is the hull of the quotients over the two parts, with an infinite
   bound where they are unbounded.  Each finite bound is rounded outward. */
CS_PRIVATE static inline interval_t interval_div_zero(interval_t a,
                                                      interval_t b) {
  if (b.lo == 0.0 && b.hi == 0.0) {
    return interval_invalid(); /* no part of b is in the domain */
  }
  if (a.lo == 0.0 && a.hi == 0.0) {
    interval_t zero = {.lo = 0.0, .hi = 0.0, .valid = true};
    return zero; /* 0 / y is 0 for every y which is not 0 */
  }

  double lo = INFINITY;
  double hi = -INFINITY;
  int save = fegetround();

  for (int part = 0; part < 2; part++) {
    /* part 0: y in (0, b.hi], part 1: y in [b.lo, 0) */
    double y = part == 0 ? b.hi : b.lo;
    if (part == 0 ? !(b.hi > 0.0) : !(b.lo < 0.0)) {
      continue;
    }
    double l = -INFINITY;
    double h = INFINITY;
    if (a.lo >= 0.0) { /* x >= 0 */
      if (part == 0) {
        fesetround(FE_DOWNWARD);
        l = iv_fence(iv_fence(a.lo) / iv_fence(y));
      } else {
        fesetround(FE_UPWARD);
        h = iv_fence(iv_fence(a.lo) / iv_fence(y));
        l = -INFINITY;
      }
    } else if (a.hi <= 0.0) { /* x <= 0 */
      if (part == 0) {
        fesetround(FE_UPWARD);
        h = iv_fence(iv_fence(a.hi) / iv_fence(y));
        l = -INFINITY;
      } else {
        fesetround(FE_DOWNWARD);
        l = iv_fence(iv_fence(a.hi) / iv_fence(y));
      }
    }
    /* otherwise a contains 0 and has both signs: unbounded both ways */
    if (isnan(l) || isnan(h)) { /* infinity / infinity */
      fesetround(save);
      return interval_invalid();
    }
    lo = fmin(lo, l);
    hi = fmax(hi, h);
  }

  fesetround(save);
  interval_t rv = {.lo = lo, .hi = hi, .valid = true};
  return rv;
}

/* A divisor which contains 0 is divided by its non-zero part, see
   interval_div_zero: the result is invalid only for the divisor [0, 0]. */
static inline interval_t interval_div(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    return interval_invalid();
  }
  if (b.lo <= 0.0 && b.hi >= 0.0) {
    return interval_div_zero(a, b);
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
    return interval_invalid();
  }

  rv.valid = true;
  return rv;
}

static inline interval_t interval_fmax(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    return interval_invalid();
  }

  /* max is exact: no rounding mode needed */
  interval_t rv = {
      .lo = fmax(a.lo, b.lo), .hi = fmax(a.hi, b.hi), .valid = true};
  return rv;
}

static inline interval_t interval_fmin(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    return interval_invalid();
  }

  /* min is exact: no rounding mode needed */
  interval_t rv = {
      .lo = fmin(a.lo, b.lo), .hi = fmin(a.hi, b.hi), .valid = true};
  return rv;
}

static inline interval_t interval_exp(interval_t a) {
  if (!a.valid) {
    return interval_invalid();
  }

  /* libm is called in the caller's rounding mode: its error bound is for
     round-to-nearest, and the one ulp of widening covers it */
  interval_t rv;
  rv.hi = interval_ulp_up(exp(a.hi));
  rv.lo = fmax(interval_ulp_down(exp(a.lo)),
               0.0); /* exp is positive: do not widen below 0 */

  rv.valid = true;
  return rv;
}

static inline interval_t interval_erf(interval_t a) {
  if (!a.valid) {
    return interval_invalid();
  }

  /* see interval_exp for why there is no rounding mode here */
  interval_t rv;
  rv.hi = fmin(interval_ulp_up(erf(a.hi)), 1.0);
  rv.lo = fmax(interval_ulp_down(erf(a.lo)), -1.0);

  rv.valid = true;
  return rv;
}

static inline interval_t interval_sqrt(interval_t a) {
  /* the domain is [0, inf): use the part of the interval inside it, and fail
   * only if none of it is */
  if (!a.valid || !(a.hi >= 0.0)) {
    return interval_invalid();
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
    return interval_invalid();
  }
  interval_t rv = {.hi = floor(a.hi), .lo = floor(a.lo), .valid = true};
  return rv;
}

static inline interval_t interval_ceil(interval_t a) {
  if (!a.valid) {
    return interval_invalid();
  }
  interval_t rv = {.hi = ceil(a.hi), .lo = ceil(a.lo), .valid = true};
  return rv;
}

static inline interval_t interval_log(interval_t a) {
  /* the domain is (0, inf): use the part of the interval inside it, and fail
   * only if none of it is */
  if (!a.valid || !(a.hi > 0.0)) {
    return interval_invalid();
  }

  /* see interval_exp for why there is no rounding mode here */
  interval_t rv;
  rv.hi = interval_ulp_up(log(a.hi));
  double lo =
      a.lo > 0 ? a.lo : 0.0; /* log(0) is -inf: the lower bound is unbounded */
  rv.lo = interval_ulp_down(log(lo));

  rv.valid = true;
  return rv;
}

static inline interval_t interval_neg(interval_t a) {
  if (!a.valid) {
    return interval_invalid();
  }

  interval_t rv = {.lo = -a.hi, .hi = -a.lo, .valid = true};
  return rv;
}

static inline interval_t interval_fabs(interval_t a) {
  if (!a.valid) {
    return interval_invalid();
  }

  if (a.lo >= 0.0) {
    return a;
  }

  if (a.hi <= 0.0) {
    return interval_neg(a);
  }

  /* negation and max are exact: no rounding mode needed */
  interval_t rv = {.lo = 0.0, .hi = fmax(-a.lo, a.hi), .valid = true};
  return rv;
}

#pragma push_macro("data_t")
#pragma push_macro("prefix")
#pragma push_macro("data_less")
#undef data_t
#undef prefix
#undef data_less
#define data_t double
#define prefix ival
#include "pqueue.h"
#undef prefix
#undef data_t
#pragma pop_macro("data_less")
#pragma pop_macro("prefix")
#pragma pop_macro("data_t")

CS_PRIVATE static inline double interval_get_key(interval_t a) {
  return fmax(fabs(a.lo), fabs(a.hi));
}

/* pqueue pops the largest key first; this reverses the order. */
CS_PRIVATE static inline int interval_key_min_first(double *a, double *b) {
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
    return interval_invalid();
  }

  for (int i = 0; i < n; i++) {
    if (!a[i].valid) {
      return interval_invalid();
    }
  }

  if (n == 1) {
    return a[0];
  }

  /* With two intervals the order cannot matter, so skip the queue. */
  if (n == 2) {
    return interval_add(a[0], a[1]);
  }

  /* Repeatedly add the two intervals of smallest magnitude.  A priority queue
     is used rather than a tree because several intervals can have the same key.
   */
  interval_t rv = interval_invalid();

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

/* x^e for the single double x, rounded outwards. */
CS_PRIVATE static inline interval_t interval_pow_point(double x, uint32_t e) {
  interval_t p = {.lo = x, .hi = x, .valid = true};
  interval_t rv = {.lo = 1.0, .hi = 1.0, .valid = true}; /* 1 is exact */
  while (e > 0) {
    if ((e & 1) == 1) {
      rv = interval_mul(rv, p);
    }
    e >>= 1;
    if (e > 0) {
      p = interval_mul(p, p);
    }
  }
  return rv;
}

/* Multiplying a by itself would treat the two copies as independent and give
   too wide a range (for example [-1,2]^2 as [-2,4]), so the power of each end
   is taken and the bounds are chosen from the shape of x^e: increasing for odd
   e and for any e on x >= 0, decreasing for even e on x <= 0. */
static inline interval_t interval_pow_uint(interval_t a, uint32_t e) {
  if (!a.valid) {
    return interval_invalid();
  }
  if (e == 0) {
    /* x^0 is 1 for every x, including 0: 0^0 is taken to be 1 */
    interval_t one = {.lo = 1.0, .hi = 1.0, .valid = true}; /* 1 is exact */
    return one;
  }
  if (a.lo >= 0.0 || a.hi <= 0.0) {
    /* All of one sign: there is no dependency problem, so multiplying the
       interval is as tight as taking the ends, with one chain of
       multiplications instead of two. */
    interval_t sq = a;
    interval_t pw = {.lo = 1.0, .hi = 1.0, .valid = true}; /* 1 is exact */
    for (uint32_t k = e; k > 0; k >>= 1) {
      if ((k & 1) == 1) {
        pw = interval_mul(pw, sq);
      }
      if (k > 1) {
        sq = interval_mul(sq, sq);
      }
    }
    /* A 0 * infinity in the products (an end at 0 and one that overflows)
       makes it invalid; the ends below still give a bound then. */
    if (pw.valid) {
      return pw;
    }
  }
  interval_t l = interval_pow_point(a.lo, e);
  interval_t h = interval_pow_point(a.hi, e);
  interval_t rv;
  if ((e & 1) == 1 || a.lo >= 0.0) {
    rv.lo = l.lo;
    rv.hi = h.hi;
  } else if (a.hi <= 0.0) {
    rv.lo = h.lo;
    rv.hi = l.hi;
  } else { /* even power of an interval which contains 0 */
    rv.lo = 0.0;
    rv.hi = fmax(l.hi, h.hi);
  }
  rv.valid = !(isnan(rv.lo) || isnan(rv.hi));
  return rv;
}

/* A negative exponent is the reciprocal of the positive power: unbounded if
   that power contains zero, invalid if it is exactly [0, 0] (interval_div). */
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
    return interval_invalid();
  }

  interval_t rv;

  int save = fegetround();

  /* start with the min and max of values from a.  libm is called in the
     caller's rounding mode, see interval_exp. */
  const double s_lo = sin(a.lo);
  const double s_hi = sin(a.hi);
  rv.hi = fmax(s_lo, s_hi);
  rv.lo = fmin(s_lo, s_hi);

  fesetround(FE_UPWARD);
  double mult_hi = iv_fence(iv_fence(a.hi) / M_PI_2);

  fesetround(FE_DOWNWARD);
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
    return interval_invalid();
  }

  interval_t rv;

  int save = fegetround();

  /* start with the min and max of values from a, see interval_sin */
  const double c_lo = cos(a.lo);
  const double c_hi = cos(a.hi);
  rv.hi = fmax(c_lo, c_hi);
  rv.lo = fmin(c_lo, c_hi);

  fesetround(FE_UPWARD);
  double mult_hi = iv_fence(iv_fence(a.hi) / M_PI_2);

  fesetround(FE_DOWNWARD);
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
