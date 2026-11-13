#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A cache from int to a reference counted struct.  Each value is shared by
   the cache and by every caller of get, and is freed when the last of them
   lets go of it:

     clone  gives the caller of get a new reference: no copy is made
     evict  drops the cache's reference when an entry leaves the cache
     update drops the cache's reference to the old value when put replaces it

   Compare with lru_demo2.c, where clone and evict copy and free strings.

   The counts here are plain ints, so this is for one thread only.
*/

/* The primes below limit.  The values can be large, which is why they are
   shared instead of copied. */
typedef struct primes {
  int refs; // number of references: the cache's and the callers'
  int limit;
  int count; // number of primes in p
  int p[];
} primes;

#define hkey_t int
#define value_t primes *
#define prefix primes

#include <lru_cache.h>

#undef prefix
#undef value_t
#undef hkey_t

/* The primes below limit, with one reference held by the caller, or NULL.

   Pretend that this is expensive. */
static primes *primes_new(int limit) {
  if (limit < 0) {
    return NULL;
  }
  char *composite = calloc((size_t) limit + 1, 1);
  if (composite == NULL) {
    return NULL;
  }
  int count = 0;
  for (int i = 2; i < limit; i++) {
    if (!composite[i]) {
      count++;
      for (long j = (long) i * i; j < limit; j += i) {
        composite[j] = 1;
      }
    }
  }

  primes *ps = malloc(sizeof(primes) + (size_t) count * sizeof(int));
  if (ps == NULL) {
    free(composite);
    return NULL;
  }
  ps->refs = 1;
  ps->limit = limit;
  ps->count = 0;
  for (int i = 2; i < limit; i++) {
    if (!composite[i]) {
      ps->p[ps->count++] = i;
    }
  }
  free(composite);
  return ps;
}

/* Take another reference to ps. */
static primes *primes_ref(primes *ps) {
  ps->refs++;
  return ps;
}

/* Let go of a reference to ps, freeing it with the last one. */
static void primes_unref(primes *ps) {
  if (--ps->refs == 0) {
    printf("    free the primes below %d\n", ps->limit);
    free(ps);
  }
}

/* Called by get: the caller receives a new reference to the stored value.
   It stays valid after the entry is evicted, and the caller must unref it. */
static int clone_primes(primes *stored, primes **out) {
  *out = primes_ref(stored);
  return 0;
}

/* Called by the cache when an entry leaves it, either to make room for a new
   entry or when the cache is destroyed.  Callers may still hold references,
   so the value is only freed if this was the last one. */
static void evict_primes(int key, primes *value) {
  printf("    evict %d (refs %d -> %d)\n", key, value->refs, value->refs - 1);
  primes_unref(value);
}

/* Called by put when the key is already in the cache: the cache keeps the
   reference it was given with current, and drops the one to previous.
   Putting the stored pointer again would call update(p, p); then the cache
   was handed a second reference to a value it already holds, so one is
   dropped either way. */
static primes *update_primes(primes *previous, primes *current) {
  primes_unref(previous);
  return current;
}

/* Returns a reference to the primes below limit, which the caller must
   unref, from the cache if possible, or NULL on an error. */
static primes *lookup(lru_primes_t *lru, int limit) {
  primes *ps;
  switch (lru_primes_get(lru, limit, &ps)) {
  case 1: /* ps is a new reference */
    printf("%6d: %4d primes (cached, refs %d)\n", limit, ps->count, ps->refs);
    return ps;

  case 0:
    {
      ps = primes_new(limit); /* the caller's reference */
      if (ps == NULL) {
        return NULL;
      }
      /* put hands a reference to the cache, so take one for it */
      if (lru_primes_put(lru, limit, primes_ref(ps)) == -1) {
        primes_unref(ps);
        primes_unref(ps);
        return NULL;
      }
      printf("%6d: %4d primes (refs %d)\n", limit, ps->count, ps->refs);
      return ps;
    }

  default:
    return NULL;
  }
}

int main(void) {
  lru_primes_t *lru = lru_primes_init(3);
  if (lru == NULL) {
    fprintf(stderr, "lru_primes_init failed.\n");
    return EXIT_FAILURE;
  }
  lru_primes_set_clone(lru, clone_primes);
  lru_primes_set_evict(lru, evict_primes);
  lru_primes_set_update(lru, update_primes);

  /* With room for 3 entries, the evictions show the least recently used
     entry leaving first.  Each value is unreferenced right away, so only the
     cache holds it and evict frees it. */
  printf("lookups:\n");
  int requests[] = {100, 1000, 100, 10000, 50000, 1000};
  for (size_t i = 0; i < sizeof(requests) / sizeof(requests[0]); i++) {
    primes *ps = lookup(lru, requests[i]);
    if (ps == NULL) {
      fprintf(stderr, "lookup of %d failed.\n", requests[i]);
      lru_primes_destroy(&lru);
      return EXIT_FAILURE;
    }
    primes_unref(ps);
  }

  /* Two callers of get share one value: no copy is made. */
  printf("\nsharing 1000:\n");
  primes *a = lookup(lru, 1000);
  primes *b = lookup(lru, 1000);
  if (a == NULL || b == NULL) {
    fprintf(stderr, "lookup of 1000 failed.\n");
    lru_primes_destroy(&lru);
    return EXIT_FAILURE;
  }
  printf("same struct: %s, largest prime below 1000: %d\n",
         a == b ? "yes" : "no", a->p[a->count - 1]);

  /* The references outlive the entry: evicting 1000 only drops the cache's
     reference, and the value is freed with the last one. */
  printf("\nevicting 1000 while it is held:\n");
  for (int limit = 200; limit <= 400; limit += 100) {
    primes *ps = lookup(lru, limit); /* the third of these evicts 1000 */
    if (ps != NULL) {
      primes_unref(ps);
    }
  }
  printf("still holding the %d primes below 1000\n", a->count);
  primes_unref(a);
  primes_unref(b);

  /* destroy calls evict on the entries left */
  printf("\ndestroy:\n");
  lru_primes_destroy(&lru);
  return EXIT_SUCCESS;
}
