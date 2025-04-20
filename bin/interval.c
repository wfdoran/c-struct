//  gcc interval.c -I ../include -o interval -lm

#include <stdio.h>
#include <stdbool.h>
#include <fenv.h>
#include <math.h>
#include <float.h>

typedef struct {
  double lo;
  double hi;
  bool valid;
} interval_t;


interval_t
interval_from_double(double x) {
  interval_t rv = {.lo = nextafter(x, -DBL_MAX), .hi = nextafter(x,DBL_MAX), .valid = true};
  return rv;
}

interval_t
interval_add(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
        
  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = a.hi + b.hi;

  fesetround(FE_DOWNWARD);
  rv.lo = a.lo + b.lo;

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_sub(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = a.hi - b.lo;

  fesetround(FE_DOWNWARD);
  rv.lo = a.lo - b.hi;

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_mul(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = a.lo * b.lo;
  temp[1] = a.lo * b.hi;
  temp[2] = a.hi * b.lo;
  temp[3] = a.hi * b.hi;
  rv.hi = fmax(fmax(fmax(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = a.lo * b.lo;
  temp[1] = a.lo * b.hi;
  temp[2] = a.hi * b.lo;
  temp[3] = a.hi * b.hi;
  rv.lo = fmin(fmin(fmin(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(save);

  rv.valid = true;
  return rv;  
}

interval_t
interval_fma(interval_t a, interval_t b, interval_t c) {
  if (!a.valid || !b.valid || !c.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = fma(a.lo, b.lo, c.hi);
  temp[1] = fma(a.lo, b.hi, c.hi);
  temp[2] = fma(a.hi, b.lo, c.hi);
  temp[3] = fma(a.hi, b.hi, c.hi);
  rv.hi = fmax(fmax(fmax(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = fma(a.lo, b.lo, c.lo);
  temp[1] = fma(a.lo, b.hi, c.lo);
  temp[2] = fma(a.hi, b.lo, c.lo);
  temp[3] = fma(a.hi, b.hi, c.lo);
  rv.lo = fmin(fmin(fmin(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(save);

  rv.valid = true;
  return rv;  
  
  
}

interval_t
interval_div(interval_t a, interval_t b) {
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
  temp[0] = a.lo / b.lo;
  temp[1] = a.lo / b.hi;
  temp[2] = a.hi / b.lo;
  temp[3] = a.hi / b.hi;
  rv.hi = fmax(fmax(fmax(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = a.lo / b.lo;
  temp[1] = a.lo / b.hi;
  temp[2] = a.hi / b.lo;
  temp[3] = a.hi / b.hi;
  rv.lo = fmin(fmin(fmin(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(save);

  rv.valid = true;
  return rv;  
}

interval_t
interval_fmax(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  if (a.lo > b.hi) {
    return a;
  }

  if (b.lo > b.hi) {
    return b;
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

interval_t
interval_fmin(interval_t a, interval_t b) {
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



interval_t
interval_exp(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = exp(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = exp(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_erf(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = erf(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = erf(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}


interval_t
interval_sqrt(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = sqrt(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = sqrt(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_floor(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  int save = fegetround();

  double x = floor(a.lo);

  fesetround(save);
  return interval_from_double(x);
}
  
interval_t
interval_ceil(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  int save = fegetround();

  double x = ceil(a.hi);

  fesetround(save);
  return interval_from_double(x);
}
  
 

interval_t
interval_log(interval_t a) {
  if (!a.valid || a.lo <= 0.0) {
    interval_t bad = {.lo = 0.0, .hi = 0.0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = log(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = log(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}

  
interval_t
interval_neg(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  interval_t rv = {.lo = -a.hi, .hi = -a.lo, .valid = true};
  return rv;
}

interval_t
interval_fabs(interval_t a) {
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
#include <tree.h>
#undef prefix
#undef data_t

static double 
interval_get_key(interval_t a) {
  return fmax(fabs(a.lo), fabs(a.hi));
}


void
interval_print(interval_t a) {
  if (!a.valid) {
    printf("[invalid]\n");
  } else {
    printf("[%.20f, %.20f]\n", a.lo, a.hi);
  }
}


interval_t
interval_add_many(int n, interval_t *a) {
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
  
  tree_ival_t *t = tree_ival_init();
  for (int i = 0; i < n; i++) {
    tree_ival_insert(t, interval_get_key(a[i]), &a[i]);
  }

  interval_t temp[n - 1];
  for (int i = 0; i < n-1; i++) {
    key_ival_value_t x = tree_ival_delete_min(t);
    key_ival_value_t y = tree_ival_delete_min(t);

    interval_t xx = *(interval_t*) x.value;
    interval_t yy = *(interval_t*) y.value;
    temp[i] = interval_add(xx, yy);

    tree_ival_insert(t, interval_get_key(temp[i]), &temp[i]);
  }
  
  tree_ival_destroy(&t);
  return temp[n - 2];
}

interval_t interval_pow_int(interval_t a, uint32_t e) {
  interval_t rv = interval_from_double(1.0);

  while (e > 0) {
    if ((e & 1) == 1) {
      rv = interval_mul(rv, a);
    }
    a = interval_mul(a,a);
    e >>= 1;
  }

  return rv;
}

interval_t interval_pow_dbl(interval_t a, interval_t b) {
  interval_t x1 = interval_log(a);
  interval_t x2 = interval_mul(x1, b);
  return interval_exp(x2);
}

#define interval_pow(x, e) _Generic((e),  \
  int: interval_pow_int,                  \
  uint32_t: interval_pow_int,             \
  interval_t: interval_pow_dbl            \
)(x,e)


interval_t interval_sin(interval_t a) {
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

  double mult_hi = a.hi / M_PI_2;
  
  fesetround(FE_DOWNWARD);
  rv.lo = fmin(sin(a.lo), sin(a.hi));

  double mult_lo = a.lo / M_PI_2;

  {
    long x = lrint(trunc((mult_lo - 1.0) / 4.0));
    long y = lrint(trunc((mult_hi - 1.0) / 4.0));
    if (x != y) {
      rv.hi = 1.0;
    }
  }

  {
    long x = lrint(trunc((mult_lo - 3.0) / 4.0));
    long y = lrint(trunc((mult_hi - 3.0) / 4.0));
    if (x != y) {
      rv.lo = -1.0;
    }
  }

  fesetround(save);
  rv.valid = true;
       
  return rv;
}

interval_t interval_cos(interval_t a) {
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

  double mult_hi = a.hi / M_PI_2;
  
  fesetround(FE_DOWNWARD);
  rv.lo = fmin(cos(a.lo), cos(a.hi));

  double mult_lo = a.lo / M_PI_2;

  {
    long x = lrint(trunc((mult_lo - 0.0) / 4.0));
    long y = lrint(trunc((mult_hi - 0.0) / 4.0));
    if (x != y) {
      rv.hi = 1.0;
    }
  }

  {
    long x = lrint(trunc((mult_lo - 2.0) / 4.0));
    long y = lrint(trunc((mult_hi - 2.0) / 4.0));
    if (x != y) {
      rv.lo = -1.0;
    }
  }

  fesetround(save);
  rv.valid = true;
       
  return rv;
}

void demo_interval_sin_cos(void) {
  interval_t a = interval_from_double(M_PI_4);

  for (int32_t i = 0; i < 10; i++) {
    interval_t m = interval_from_double((double) i);
    interval_t b = interval_mul(a, m);
    interval_print(interval_sin(b));
  }
  printf("\n");

  for (int32_t i = 0; i < 10; i++) {
    interval_t m = interval_from_double((double) i);
    interval_t b = interval_mul(a, m);
    interval_print(interval_cos(b));
  }
  printf("\n");
}

void demo_interval_add_many(void) {
  int n = 10;

  interval_t a[n];
  for (int i = 0; i < n; i++) {
    a[i] = interval_from_double(1.0 + i * i);
  }

  interval_t sum = interval_add_many(n, a);
  interval_print(sum);

  for (int i = 1; i < n; i++) {
    a[0] = interval_add(a[0], a[i]);
  }
  interval_print(a[0]);
  printf("\n");
}

void demo_interval_pow(void) {
  interval_t x = interval_from_double(1.7);
  interval_t y = interval_pow(x, 10);
  interval_t z = interval_from_double(10.0);
  interval_t w = interval_pow(x, z);
  interval_print(y);
  interval_print(w);
  printf("\n");
}

void demo_interval_arith(void) {
  interval_t a = interval_from_double(4.0);
  interval_t b = interval_from_double(2.1);
  interval_t c = interval_add(a, b);
  interval_print(c);
  interval_t d = interval_sub(a,b);
  interval_print(d);
  interval_t e = interval_mul(a,b);
  interval_print(e);
  interval_t f = interval_div(a,b);
  interval_print(f);
  printf("\n");
}

void demo_sqrt(void) {
  interval_t a = interval_from_double(2.0);
  interval_t b = interval_sqrt(a);
  interval_t c = interval_mul(b,b);
  interval_t d = interval_sub(c, a);
  interval_t e = interval_fabs(d);
  interval_print(b);
  interval_print(c);
  interval_print(d);
  interval_print(e);
  printf("\n");
}

void demo_interval_fma() {
  interval_t a = interval_from_double(2000000000000000.0);
  interval_t b = interval_from_double(0.0000000000000005);
  interval_t c = interval_from_double(-1.0);

  interval_t x = interval_fma(a,b,c);
  interval_t y = interval_add(interval_mul(a,b), c);

  interval_print(x);
  interval_print(y);
  printf("\n");
}

int main(void) {
  // demo_interval_arith();
  // demo_interval_add_many();
  // demo_interval_pow();
  // demo_sqrt();
  // demo_interval_sin_cos();
  demo_interval_fma();
  
  return 0;
}
