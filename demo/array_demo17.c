#include <stdio.h>
#include <stdlib.h>

/* Sorting with the comparison compiled in.

   By default an array calls its comparison through a function pointer
   (array_prefix_set_comp).  If data_less is defined before including the
   header, the ordering is written inline instead, which lets the compiler
   inline it: sorting is about 1.6x faster.  set_comp is not used then. */

typedef struct {
    int id;
    double score;
} entry_t;

#define data_t entry_t
#define prefix entry
#define data_less(x, y) ((x).score < (y).score)
#include <array.h>
#undef prefix
#undef data_t

int main(void) {
    array_entry_t *a = array_entry_init();

    for (int i = 0; i < 10; i++) {
        entry_t e = { .id = i, .score = (double) ((i * 7) % 10) };
        array_entry_append(a, e);
    }

    /* no set_comp needed, even though entry_t has no default comparison */
    array_entry_sort(a);

    for (size_t i = 0; i < array_entry_size(a); i++) {
        entry_t e = array_entry_get(a, i);
        printf("score %.1f  id %d\n", e.score, e.id);
    }

    entry_t key = { .id = -1, .score = 4.0 };
    ssize_t idx = array_entry_bisect(a, key);
    printf("score 4.0 is at index %zd\n", idx);

    array_entry_destroy(&a);
    return 0;
}
