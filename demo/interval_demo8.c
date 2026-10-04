/* Converting integers which are not exactly representable as doubles (2^53 +-
 * 1) gives intervals which contain them. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with "
                    "-frounding-math\n");
    return 1;
  }

  int64_t base = UINT64_C(1) << 53;
  int64_t delta = UINT64_C(1);

  interval_t a = interval_from_int64(base - delta);
  interval_t b = interval_from_int64(base + delta);

  interval_print(a);
  interval_print(b);

  return 0;
}
