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
