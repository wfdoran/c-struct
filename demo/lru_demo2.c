#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A cache from int to char*.  The cache owns the strings stored in it:

     evict  frees a string when its entry leaves the cache
     clone  gives each caller of get a copy of its own
     update frees the old string when put replaces a value
*/

#define hkey_t int
#define value_t char *
#define prefix words

#include <lru_cache.h>

#undef prefix
#undef value_t
#undef hkey_t

static char *copy_string(const char *s) {
  size_t len = strlen(s) + 1;
  char *copy = malloc(len);
  if (copy != NULL) {
    memcpy(copy, s, len);
  }
  return copy;
}

/* Spells out 0 <= n <= 999 in a newly allocated string, or returns NULL.

   Pretend that this is expensive. */
static char *number_to_words(int n) {
  static const char *ones[] = {"zero",    "one",       "two",      "three",
                               "four",    "five",      "six",      "seven",
                               "eight",   "nine",      "ten",      "eleven",
                               "twelve",  "thirteen",  "fourteen", "fifteen",
                               "sixteen", "seventeen", "eighteen", "nineteen"};
  static const char *tens[] = {"",      "",      "twenty",  "thirty", "forty",
                               "fifty", "sixty", "seventy", "eighty", "ninety"};
  if (n < 0 || n > 999) {
    return NULL;
  }

  char buf[64] = "";
  if (n >= 100) {
    strcat(buf, ones[n / 100]);
    strcat(buf, " hundred");
    n %= 100;
    if (n == 0) {
      return copy_string(buf);
    }
    strcat(buf, " ");
  }
  if (n < 20) {
    strcat(buf, ones[n]);
  } else {
    strcat(buf, tens[n / 10]);
    if (n % 10 != 0) {
      strcat(buf, "-");
      strcat(buf, ones[n % 10]);
    }
  }
  return copy_string(buf);
}

/* Called by the cache when an entry leaves it, either to make room for a new
   entry or when the cache is destroyed.  The key is an int, so only the value
   needs to be freed. */
static void evict_words(int key, char *value) {
  printf("    evict %d \"%s\"\n", key, value);
  free(value);
}

/* Called by get: the caller receives its own copy, which stays valid after
   the entry is evicted, and which the caller must free. */
static int clone_words(char *stored, char **out) {
  *out = copy_string(stored);
  return *out == NULL ? -1 : 0;
}

/* Called by put when the key is already in the cache.  evict is not called
   then, so the old string is freed here.  Putting the stored pointer again
   would call update(p, p), hence the check. */
static char *update_words(char *previous, char *current) {
  if (previous != current) {
    free(previous);
  }
  return current;
}

/* Returns the words for n as a string owned by the caller, from the cache if
   possible, or NULL on an error. */
static char *lookup(lru_words_t *lru, int n) {
  char *words;
  switch (lru_words_get(lru, n, &words)) {
  case 1: /* words is a clone */
    printf("%4d \"%s\" (cached)\n", n, words);
    return words;

  case 0:
    {
      words = number_to_words(n);
      if (words == NULL) {
        return NULL;
      }
      /* the caller needs its own copy, since put hands words to the cache */
      char *mine = copy_string(words);
      if (mine == NULL) {
        free(words);
        return NULL;
      }
      if (lru_words_put(lru, n, words) == -1) {
        free(words);
        free(mine);
        return NULL;
      }
      printf("%4d \"%s\"\n", n, mine);
      return mine;
    }

  default:
    return NULL;
  }
}

int main(void) {
  lru_words_t *lru = lru_words_init(3);
  if (lru == NULL) {
    fprintf(stderr, "lru_words_init failed.\n");
    return EXIT_FAILURE;
  }
  lru_words_set_evict(lru, evict_words);
  lru_words_set_clone(lru, clone_words);
  lru_words_set_update(lru, update_words);

  /* With room for 3 entries, the evictions show the least recently used
     entry leaving first. */
  printf("lookups:\n");
  int requests[] = {7, 42, 7, 115, 300, 42, 7, 999};
  for (size_t i = 0; i < sizeof(requests) / sizeof(requests[0]); i++) {
    char *words = lookup(lru, requests[i]);
    if (words == NULL) {
      fprintf(stderr, "lookup of %d failed.\n", requests[i]);
      lru_words_destroy(&lru);
      return EXIT_FAILURE;
    }
    free(words); /* our copy, not the one in the cache */
  }

  /* A string from get outlives its entry in the cache. */
  printf("\nholding on to 7:\n");
  char *held = lookup(lru, 7);
  for (int n = 500; n < 503; n++) {
    free(lookup(lru, n)); /* the third of these evicts 7 */
  }
  if (held != NULL) {
    printf("still holding \"%s\"\n", held);
    free(held);
  }

  /* destroy calls evict on the entries left */
  printf("\ndestroy:\n");
  lru_words_destroy(&lru);
  return EXIT_SUCCESS;
}
