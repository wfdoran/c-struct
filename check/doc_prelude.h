/* Included before every example of the documentation when it is checked (see
   doc_examples.sh).  The examples write a placeholder where the user supplies a
   type of their own; this gives those names a meaning, and the standard headers
   that a longer program would include. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int key_type; /* doc/tree.md: "the type of the keys" */

typedef struct { /* doc/hash_table.md, doc/phash_table.md: a struct key */
  int32_t x;
  int32_t y;
} cell_t;
