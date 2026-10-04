#include <stdio.h>
#include <stdlib.h>

/* A tree with a compiled in ordering: points ordered by x, then by y.
   data_less replaces the comparison function pointer (tree_prefix_set_comp). */

typedef struct {
    int x;
    int y;
} point_t;

#define data_t point_t
#define prefix point
#define data_less(a, b) \
    ((a).x < (b).x || ((a).x == (b).x && (a).y < (b).y))
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    tree_point_t *t = tree_point_init();

    for (int i = 0; i < 12; i++) {
        point_t p = { .x = (i * 5) % 4, .y = (i * 7) % 5 };
        tree_point_insert(t, p, NULL);
    }
    printf("points: %zu\n", tree_point_size(t));

    point_t p;
    titer_point_t it;
    for (int32_t rc = tree_point_first(t, &it, &p, NULL); rc == 0;
         rc = tree_point_next(&it, &p, NULL)) {
        printf("(%d, %d) ", p.x, p.y);
    }
    printf("\n");

    point_t q = { .x = 2, .y = 3 };
    printf("(2, 3) %s\n", tree_point_retrieve(t, q).found ? "is in the tree"
                                                            : "is not in the tree");
    tree_point_destroy(&t);
    return 0;
}
