#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define hkey_t int
#define value_t double
#define prefix sqrt

#include <lru_cache.h>

#undef prefix
#undef value_t
#undef hkey_t


/*
   If we have recently called expensive_sqrt, we would like to look it
   up the LRU Cache instead of calling this again.

   Note: this routine may be faster than an LRU lookup.  Pretend that
   it isn't.
*/
   
static double expensive_sqrt(int n) {
  if (n <= 0)  return 0.0;
    
  double a = (double)n;
  double x = a;                /* initial guess >= sqrt(a) for a >= 1 */
  double prev;

  do {
    prev = x;
    x = 0.5 * (x + a / x);     /* Newton step for f(x) = x^2 - a */
  } while (x < prev);          /* strictly decreasing until converged */

  return x;
}


int main(void) {
  srand(time(NULL));
  
  lru_sqrt_t *lru = lru_sqrt_init(10);
  if (lru == NULL) {
    printf("lru_sqrt_init failed.\n");
    return EXIT_FAILURE;
  }

  for (int iter = 0; iter < 100; iter++) {
    /* Pick some biased values.  Low density n are
       more likely to be selected. */
    int a = rand() & 0xff;
    int b = rand() & 0xff;
    int n = a & b;

    /* Hook the choice at iter 10, 15, and 70.

       At iter 15, sqrt(1000) will be in the cache, but
       by iter 70 it will most likely have been evicted. */
    if (iter == 10 || iter == 15 || iter == 70) {
      n = 1000;
    }

    double v;

    switch(lru_sqrt_get(lru, n, &v)) {
    case 1:
      printf("%4d %4d %12.8f (cached)\n", iter, n, v);
      break;

    case 0:
      { 
	v = expensive_sqrt(n);
	int rc = lru_sqrt_put(lru, n, v);
	if (rc == -1) {
	  printf("lru_sqrt_put failed.\n");
	  return EXIT_FAILURE;
	}
	printf("%4d %4d %12.8f\n", iter, n, v);
      }
      break;

    default:
      printf("lru_sqrt_get failed\n");
      return EXIT_FAILURE;
    }
  }

  lru_sqrt_destroy(&lru);

  return 0;
}
