#include <check.h>
#include <math.h>
#include <interval.h>

// The sine and cosine of an interval which contains a maximum or a minimum must reach 1 or -1.
// The interval is the two doubles on either side of the extremum, found in long double.  Before
// the quotient by pi/2 was widened, a lower end just before the extremum was often judged to lie
// after it once the argument was about 1e9 or more.
START_TEST(interval_test1)

CHECK(interval_rounding_is_honored());

const long double PI = 3.14159265358979323846264338327950288L;
double bases[] = {1e2, 1e4, 1e6, 1e8, 1e9, 1e10, 1e12, 1e13};
long tested = 0;
for (int bi = 0; bi < 8; bi++) {
  for (int sign = -1; sign <= 1; sign += 2) {
    for (int which = 0; which < 4; which++) {   // sin max, sin min, cos max, cos min
      long k0 = (long) (bases[bi] / 6.28318530718);
      for (long k = k0; k < k0 + 2000; k++) {
        long double kk = sign * (long double) k;
        long double xs = which == 0   ? PI / 2 + 2 * PI * kk
                         : which == 1 ? 3 * PI / 2 + 2 * PI * kk
                         : which == 2 ? 2 * PI * kk
                                      : PI + 2 * PI * kk;
        double lo = (double) xs;
        if ((long double) lo > xs) {
          lo = nextafter(lo, -INFINITY);
        }
        double hi = nextafter(lo, INFINITY);
        double ulp = hi - lo;
        if (!((long double) lo < xs - 0.05L * ulp && (long double) hi > xs + 0.05L * ulp)) {
          continue;   // the extremum is too close to an end to be sure it is inside
        }
        interval_t a = {.lo = lo, .hi = hi, .valid = true};
        interval_t r = which < 2 ? interval_sin(a) : interval_cos(a);
        tested++;
        CHECK(r.valid);
        if (which == 0 || which == 2) {
          CHECK(r.hi >= 1.0);
        } else {
          CHECK(r.lo <= -1.0);
        }
        CHECK(r.lo >= -1.0 && r.hi <= 1.0);
      }
    }
  }
}
CHECK(tested > 100000);

END_TEST

// the interval of a sine or cosine contains the value at sampled points, for intervals of many
// sizes and positions
START_TEST(interval_test2)

seed_rand(2);
double mags[] = {0.5, 3, 40, 1e3, 1e6, 1e9};
for (int mi = 0; mi < 6; mi++) {
  for (int i = 0; i < 4000; i++) {
    double x = ((double) (get_rand() >> 8) / 16777216.0 * 2 - 1) * mags[mi];
    double w = (double) (get_rand() >> 8) / 16777216.0 * (i % 4 == 0 ? 8 : i % 4 == 1 ? 1e-3 : 1e-9);
    interval_t a = {.lo = x, .hi = x + w, .valid = true};
    interval_t s = interval_sin(a);
    interval_t c = interval_cos(a);
    CHECK(s.valid && c.valid && s.lo <= s.hi && c.lo <= c.hi);
    for (int j = 0; j <= 8; j++) {
      long double xj = (long double) a.lo + ((long double) a.hi - (long double) a.lo) * j / 8;
      long double sj = sinl(xj), cj = cosl(xj);
      CHECK(s.lo <= sj && sj <= s.hi);
      CHECK(c.lo <= cj && cj <= c.hi);
    }
  }
}

END_TEST

// the basic operations enclose the exact result
START_TEST(interval_test3)

interval_t a = interval_from_double(1.0 / 3.0);
interval_t b = interval_from_double(0.1);
interval_t s = interval_add(a, b);
CHECK(s.valid && s.lo < (1.0L / 3 + 0.1L) && (1.0L / 3 + 0.1L) < s.hi);
interval_t p = interval_mul(a, b);
CHECK(p.valid && p.lo < p.hi && p.lo <= (1.0L / 3) * 0.1L && (1.0L / 3) * 0.1L <= p.hi);
interval_t q = interval_div(a, b);
CHECK(q.valid && q.lo <= (1.0L / 3) / 0.1L && (1.0L / 3) / 0.1L <= q.hi);
CHECK(!interval_div(a, interval_from_double(0.0)).valid);

// the domain rule: use the part of the interval which is in the domain
interval_t neg_pos = {.lo = -1.0, .hi = 4.0, .valid = true};
interval_t r = interval_sqrt(neg_pos);
CHECK(r.valid && r.lo == 0.0 && r.hi >= 2.0);
interval_t all_neg = {.lo = -4.0, .hi = -1.0, .valid = true};
CHECK(!interval_sqrt(all_neg).valid);
interval_t l = interval_log(neg_pos);
CHECK(l.valid && l.lo == -INFINITY && l.hi >= log(4.0));
CHECK(!interval_log(all_neg).valid);
interval_t e = interval_exp(interval_from_double(-1000.0));
CHECK(e.valid && e.lo == 0.0);

// invalid intervals stay invalid
interval_t bad = {.lo = 0, .hi = 0, .valid = false};
CHECK(!interval_add(bad, a).valid && !interval_sin(bad).valid && !interval_exp(bad).valid);
CHECK(!interval_from_double(NAN).valid);

END_TEST

// random interval with lo <= hi inside [-mag, mag] (or [tiny, mag] when positive)
static interval_t random_interval(double mag, bool positive) {
  double u = (double) (get_rand() >> 8) / 16777216.0;
  double v = (double) (get_rand() >> 8) / 16777216.0;
  double x = positive ? mag * (u * u + 1e-3) : (u * 2 - 1) * mag;
  double w = (get_rand() % 4 == 0) ? 0.0 : v * v * mag;
  interval_t r = {.lo = x, .hi = x + w, .valid = true};
  return r;
}

// the k-th of n + 1 points of an interval, in long double
static long double sample_point(interval_t a, int k, int n) {
  return (long double) a.lo + ((long double) a.hi - (long double) a.lo) * k / n;
}

// sub, fma, fmax, fmin, neg, fabs, floor, ceil enclose the exact result at sample points
START_TEST(interval_test4)

seed_rand(4);
for (int i = 0; i < 3000; i++) {
  interval_t a = random_interval(100, false);
  interval_t b = random_interval(100, false);
  interval_t c = random_interval(100, false);
  interval_t d = interval_sub(a, b);
  interval_t f = interval_fma(a, b, c);
  interval_t mx = interval_fmax(a, b);
  interval_t mn = interval_fmin(a, b);
  interval_t ng = interval_neg(a);
  interval_t ab = interval_fabs(a);
  interval_t fl = interval_floor(a);
  interval_t ce = interval_ceil(a);
  CHECK(d.valid && f.valid && mx.valid && mn.valid && ng.valid && ab.valid && fl.valid && ce.valid);
  CHECK(d.lo <= d.hi && f.lo <= f.hi && mx.lo <= mx.hi && mn.lo <= mn.hi);
  for (int j = 0; j <= 4; j++) {
    long double x = sample_point(a, j, 4);
    long double y = sample_point(b, (j * 3) % 5, 4);
    long double z = sample_point(c, (j * 2) % 5, 4);
    CHECK(d.lo <= x - y && x - y <= d.hi);
    CHECK(f.lo <= x * y + z && x * y + z <= f.hi);
    CHECK(mx.lo <= (x > y ? x : y) && (x > y ? x : y) <= mx.hi);
    CHECK(mn.lo <= (x < y ? x : y) && (x < y ? x : y) <= mn.hi);
    CHECK(ng.lo <= -x && -x <= ng.hi);
    CHECK(ab.lo <= fabsl(x) && fabsl(x) <= ab.hi);
    CHECK(fl.lo <= floorl(x) && floorl(x) <= fl.hi);
    CHECK(ce.lo <= ceill(x) && ceill(x) <= ce.hi);
  }
}

// neg and fabs are exact, and fabs is never negative
interval_t m = {.lo = -3.0, .hi = 2.0, .valid = true};
CHECK(interval_neg(m).lo == -2.0 && interval_neg(m).hi == 3.0);
CHECK(interval_fabs(m).lo == 0.0 && interval_fabs(m).hi == 3.0);
interval_t bad = {.lo = 0, .hi = 0, .valid = false};
CHECK(!interval_sub(bad, m).valid && !interval_fma(m, m, bad).valid);
CHECK(!interval_fmax(bad, m).valid && !interval_fmin(m, bad).valid);
CHECK(!interval_neg(bad).valid && !interval_fabs(bad).valid);
CHECK(!interval_floor(bad).valid && !interval_ceil(bad).valid);

END_TEST

// exp, erf, sqrt, log and the powers enclose the exact result at sample points
START_TEST(interval_test5)

seed_rand(5);
for (int i = 0; i < 3000; i++) {
  interval_t a = random_interval(20, false);
  interval_t p = random_interval(50, true);
  interval_t e = interval_exp(a);
  interval_t er = interval_erf(a);
  interval_t sq = interval_sqrt(p);
  interval_t lg = interval_log(p);
  CHECK(e.valid && er.valid && sq.valid && lg.valid);
  for (int j = 0; j <= 4; j++) {
    long double x = sample_point(a, j, 4);
    long double y = sample_point(p, j, 4);
    CHECK(e.lo <= expl(x) && expl(x) <= e.hi);
    CHECK(er.lo <= erfl(x) && erfl(x) <= er.hi);
    CHECK(sq.lo <= sqrtl(y) && sqrtl(y) <= sq.hi);
    CHECK(lg.lo <= logl(y) && logl(y) <= lg.hi);
  }

  // integer powers, also negative ones (away from zero) and a power of an interval
  interval_t base = random_interval(3, true);
  for (int ex = 0; ex <= 7; ex++) {
    interval_t pu = interval_pow_uint(base, (uint32_t) ex);
    interval_t pi = interval_pow_int(base, -ex);
    CHECK(pu.valid && pi.valid);
    for (int j = 0; j <= 2; j++) {
      long double x = sample_point(base, j, 2);
      CHECK(pu.lo <= powl(x, ex) && powl(x, ex) <= pu.hi);
      CHECK(pi.lo <= powl(x, -ex) && powl(x, -ex) <= pi.hi);
    }
  }

  // a ^ b for a positive a
  interval_t pb = random_interval(2, false);
  interval_t pw = interval_pow_dbl(base, pb);
  CHECK(pw.valid);
  for (int j = 0; j <= 2; j++) {
    long double x = sample_point(base, j, 2);
    long double y = sample_point(pb, (j * 2) % 3, 2);
    CHECK(pw.lo <= powl(x, y) && powl(x, y) <= pw.hi);
  }
}

// a negative exponent of an interval which contains zero is invalid
interval_t z = {.lo = -1.0, .hi = 1.0, .valid = true};
CHECK(!interval_pow_int(z, -1).valid);
CHECK(interval_pow_uint(z, 0).valid);

// x^0 is exactly [1, 1], an exact base gives an exact power, and an invalid
// base gives an invalid result even for the exponent 0
CHECK(interval_pow_uint(z, 0).lo == 1.0 && interval_pow_uint(z, 0).hi == 1.0);
interval_t two = {.lo = 2.0, .hi = 2.0, .valid = true};
CHECK(interval_pow_uint(two, 3).lo == 8.0 && interval_pow_uint(two, 3).hi == 8.0);
interval_t bad = {.lo = 0, .hi = 0, .valid = false};
CHECK(!interval_pow_uint(bad, 0).valid);
CHECK(!interval_pow_int(bad, 0).valid);
CHECK(!interval_pow_int(bad, -2).valid);

END_TEST

// from_int64 encloses large integers exactly, add_many encloses the sum, and the endpoints
START_TEST(interval_test6)

int64_t big[] = {0, 1, -1, 1000, (INT64_C(1) << 53), (INT64_C(1) << 53) + 1, INT64_MAX, INT64_MIN,
                 INT64_MAX - 1, INT64_MIN + 1, 123456789012345678};
for (size_t i = 0; i < sizeof(big) / sizeof(big[0]); i++) {
  interval_t r = interval_from_int64(big[i]);
  CHECK(r.valid && r.lo <= r.hi);
  CHECK((long double) r.lo <= (long double) big[i] && (long double) big[i] <= (long double) r.hi);
  if (big[i] > -(INT64_C(1) << 53) && big[i] < (INT64_C(1) << 53)) {
    CHECK(r.lo == r.hi); // exactly representable
  }
}

seed_rand(6);
for (int n = 1; n <= 40; n++) {
  interval_t v[40];
  long double lo = 0, hi = 0, mid = 0;
  for (int k = 0; k < n; k++) {
    v[k] = random_interval(1e3, false);
    lo += v[k].lo;
    hi += v[k].hi;
    mid += sample_point(v[k], 1, 2);
  }
  interval_t s = interval_add_many(n, v);
  CHECK(s.valid && s.lo <= s.hi);
  CHECK(s.lo <= lo + 1e-9L * fabsl(lo) + 1e-9L && s.lo <= mid && mid <= s.hi);
  CHECK(s.hi >= hi - 1e-9L * fabsl(hi) - 1e-9L);
}
CHECK(!interval_add_many(0, NULL).valid);
interval_t one[1] = {{.lo = 1.0, .hi = 2.0, .valid = true}};
CHECK(interval_add_many(1, one).lo == 1.0 && interval_add_many(1, one).hi == 2.0);
interval_t with_bad[2] = {{.lo = 1.0, .hi = 2.0, .valid = true}, {.lo = 0, .hi = 0, .valid = false}};
CHECK(!interval_add_many(2, with_bad).valid);

// the helpers which order the queue
interval_t k1 = {.lo = -5.0, .hi = 2.0, .valid = true};
CHECK(interval_get_key(k1) == 5.0);
double small = 1.0, large = 2.0;
CHECK(interval_key_min_first(&small, &large) > 0 && interval_key_min_first(&large, &small) < 0);

END_TEST

// x + y and x - y rounded in the given mode, as a reference for add and sub
static double add_rounded(double x, double y, int mode) {
  volatile double vx = x, vy = y;
  int save = fegetround();
  fesetround(mode);
  volatile double r = vx + vy;
  fesetround(save);
  return r;
}

static double sub_rounded(double x, double y, int mode) {
  volatile double vx = x, vy = y;
  int save = fegetround();
  fesetround(mode);
  volatile double r = vx - vy;
  fesetround(save);
  return r;
}

// same value, and the same sign for zeros
static bool same_double(double x, double y) {
  return x == y && signbit(x) == signbit(y);
}

// add and sub compute the lower bound with one mode switch; it must be exactly
// the bound rounded down, also for signed zeros, infinities and inexact sums
START_TEST(interval_test7)

const double special[] = {0.0, -0.0, 1.0, -1.0, 0.1, -0.1, 1e308, -1e308, DBL_MIN, -DBL_MIN,
                          DBL_TRUE_MIN, -DBL_TRUE_MIN, INFINITY, -INFINITY};
const int ns = sizeof(special) / sizeof(special[0]);
for (int i = 0; i < ns; i++) {
  for (int j = 0; j < ns; j++) {
    double x = special[i], y = special[j];
    interval_t a = {.lo = x, .hi = x, .valid = true};
    interval_t b = {.lo = y, .hi = y, .valid = true};
    interval_t s = interval_add(a, b);
    interval_t d = interval_sub(a, b);
    if (!isnan(add_rounded(x, y, FE_DOWNWARD))) {
      CHECK(same_double(s.lo, add_rounded(x, y, FE_DOWNWARD)));
      CHECK(same_double(s.hi, add_rounded(x, y, FE_UPWARD)));
    }
    if (!isnan(sub_rounded(x, y, FE_DOWNWARD))) {
      CHECK(same_double(d.lo, sub_rounded(x, y, FE_DOWNWARD)));
      CHECK(same_double(d.hi, sub_rounded(x, y, FE_UPWARD)));
    }
  }
}

seed_rand(7);
for (int i = 0; i < 100000; i++) {
  interval_t a = random_interval(100, false);
  interval_t b = random_interval(1e-3, false);
  interval_t s = interval_add(a, b);
  interval_t d = interval_sub(a, b);
  CHECK(same_double(s.lo, add_rounded(a.lo, b.lo, FE_DOWNWARD)));
  CHECK(same_double(s.hi, add_rounded(a.hi, b.hi, FE_UPWARD)));
  CHECK(same_double(d.lo, sub_rounded(a.lo, b.hi, FE_DOWNWARD)));
  CHECK(same_double(d.hi, sub_rounded(a.hi, b.lo, FE_UPWARD)));
}

END_TEST

// interval_invalid is invalid, and is what the functions return when there is no result
START_TEST(interval_test8)

interval_t v = interval_invalid();
CHECK(!v.valid);
interval_t one = {.lo = 1.0, .hi = 1.0, .valid = true};
CHECK(!interval_add(v, one).valid);
CHECK(!interval_sqrt(interval_neg(one)).valid);
CHECK(!interval_from_double(NAN).valid);

END_TEST

// integer powers of intervals of either sign enclose x^e, and are as tight as the shape of x^e
// allows: an even power is never negative, and an odd power of [lo, hi] starts near lo^e
START_TEST(interval_test9)

seed_rand(7);
for (int i = 0; i < 4000; i++) {
  interval_t a = random_interval(i % 3 == 0 ? 0.9 : 5, false);
  for (uint32_t e = 0; e <= 11; e++) {
    interval_t p = interval_pow_uint(a, e);
    CHECK(p.valid && p.lo <= p.hi);
    for (int j = 0; j <= 6; j++) {
      long double x = sample_point(a, j, 6);
      CHECK(p.lo <= powl(x, e) && powl(x, e) <= p.hi);
    }
    if (e % 2 == 0) {
      CHECK(p.lo >= 0.0);
    }
    if (e % 2 == 1) {
      // monotone: the bounds come from the ends, so they are within a few ulps of lo^e and hi^e
      long double lo = powl((long double) a.lo, e), hi = powl((long double) a.hi, e);
      CHECK(fabsl((long double) p.lo - lo) <= 1e-12L * fabsl(lo) + 1e-300L);
      CHECK(fabsl((long double) p.hi - hi) <= 1e-12L * fabsl(hi) + 1e-300L);
    }
  }
}

interval_t s = {.lo = -1.0, .hi = 2.0, .valid = true};
interval_t sq = interval_pow_uint(s, 2);
CHECK(sq.lo == 0.0 && sq.hi >= 4.0 && sq.hi < 4.0001);
interval_t c = {.lo = -1.0, .hi = 4.0, .valid = true};
interval_t cu = interval_pow_uint(c, 3);
CHECK(cu.lo <= -1.0 && cu.lo > -1.0001 && cu.hi >= 64.0 && cu.hi < 64.001);
interval_t neg = {.lo = -3.0, .hi = -2.0, .valid = true};
interval_t ne = interval_pow_uint(neg, 2);   // even power on the negative side decreases
CHECK(ne.lo <= 4.0 && ne.lo > 3.999 && ne.hi >= 9.0 && ne.hi < 9.001);
CHECK(interval_pow_uint(s, 0).lo == 1.0 && interval_pow_uint(s, 0).hi == 1.0);
interval_t bad = {.lo = 0, .hi = 0, .valid = false};
CHECK(!interval_pow_uint(bad, 3).valid && !interval_pow_uint(bad, 0).valid);
// negative exponents: the reciprocal of the tight power
interval_t pos = {.lo = 2.0, .hi = 3.0, .valid = true};
interval_t inv = interval_pow_int(pos, -2);
CHECK(inv.lo <= 1.0 / 9.0 && inv.hi >= 1.0 / 4.0 && inv.hi < 0.2501);
CHECK(!interval_pow_int(s, -2).valid);   // the power contains 0

END_TEST

// a NaN bound (infinity minus infinity) makes add and sub invalid, like mul and div
START_TEST(interval_test10)

interval_t ninf = {.lo = -INFINITY, .hi = -INFINITY, .valid = true};
interval_t pinf = {.lo = INFINITY, .hi = INFINITY, .valid = true};
CHECK(!interval_add(ninf, pinf).valid);
CHECK(!interval_add(pinf, ninf).valid);
CHECK(!interval_sub(pinf, pinf).valid);
CHECK(!interval_sub(ninf, ninf).valid);

// the half-infinite intervals which the library itself makes still work
interval_t up = {.lo = 0.0, .hi = INFINITY, .valid = true};
interval_t down = {.lo = -INFINITY, .hi = 0.0, .valid = true};
interval_t all = {.lo = -INFINITY, .hi = INFINITY, .valid = true};
interval_t r = interval_add(up, down);
CHECK(r.valid && r.lo == -INFINITY && r.hi == INFINITY);
r = interval_sub(up, up);
CHECK(r.valid && r.lo == -INFINITY && r.hi == INFINITY);
r = interval_add(all, all);
CHECK(r.valid && r.lo == -INFINITY && r.hi == INFINITY);
r = interval_sub(all, interval_from_double(1.0));
CHECK(r.valid && r.lo == -INFINITY && r.hi == INFINITY);
r = interval_add(interval_from_double(INFINITY), interval_from_double(-INFINITY));
CHECK(r.valid && r.lo == -INFINITY && r.hi == INFINITY);

END_TEST
