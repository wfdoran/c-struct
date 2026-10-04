#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* A hash table whose hash and equality are compiled in.  Without these
   macros, a key such as a struct needs hash_prefix_set_hash and
   hash_prefix_set_comp, which are called through function pointers. */

typedef struct {
    int32_t x;
    int32_t y;
} cell_t;

#define hkey_t cell_t
#define value_t int
#define prefix cell
#define hkey_hash(k) (((uint64_t) (uint32_t) (k).x << 32) | (uint32_t) (k).y)
#define hkey_equal(a, b) ((a).x == (b).x && (a).y == (b).y)
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

int main(void) {
    htable_cell_t *h = hash_cell_init(0);

    for (int i = 0; i < 100; i++) {
        cell_t c = { .x = i % 10, .y = i / 10 };
        hash_cell_put(h, c, i);
    }
    printf("cells: %zu\n", hash_cell_size(h));

    cell_t c = { .x = 3, .y = 7 };
    int v;
    if (hash_cell_get(h, c, &v) == 1) {
        printf("(3, 7) -> %d\n", v);
    }
    c.x = 30;
    printf("(30, 7) %s\n", hash_cell_get(h, c, &v) == 1 ? "found" : "missing");

    hash_cell_destroy(&h);
    return 0;
}
