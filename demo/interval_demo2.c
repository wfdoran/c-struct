/* interval_add_many adds the smallest intervals first, so rounding errors stay small.  The second line adds the same numbers in order. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with -frounding-math\n");
    return 1;
  }

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

  return 0;
}
