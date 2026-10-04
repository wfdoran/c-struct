/* The continued fraction of pi, until the interval is too wide to tell the next
 * term. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with "
                    "-frounding-math\n");
    return 1;
  }

  /* https://oeis.org/A001203 lists the terms: 3, 7, 15, 1, 292, 1, 1, 1, 2, 1,
   * 3, 1, 14, 2, 1, 1, 2, 2, 2, 2, 1 */

  interval_t x = interval_from_double(M_PI);
  interval_t one = interval_from_double(1.0);

  int32_t should_be[] = {3, 7,  15, 1, 292, 1, 1, 1, 2, 1, 3,
                         1, 14, 2,  1, 1,   2, 2, 2, 2, 1};
  int32_t max_steps = sizeof(should_be) / sizeof(should_be[0]);

  for (int i = 0; i < max_steps; i++) {
    interval_t a = interval_floor(x);
    if (x.hi - x.lo > .5) {
      printf("%4d %8d : %8.0f %8.0f\n", i, should_be[i], rint(a.lo),
             rint(a.hi));
      printf("Lost accuracy\n");
      break;
    } else {
      printf("%4d %8d : %8.0f\n", i, should_be[i], rint(a.lo));
    }

    x = interval_sub(x, a);
    x = interval_div(one, x);
  }

  return 0;
}
