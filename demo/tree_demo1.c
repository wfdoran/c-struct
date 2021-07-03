#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

void node_print(int32_t x, void *unused) {
  printf("%d", x);
}

int main(void) {
    srand(time(NULL));
    int32_t num_items = 100;
    
    tree_int32_t *t = tree_int32_init();
    
    for (int i = 0; i < num_items; i++) {
        int32_t val = (rand() >> 3) & 0xffff;
        tree_int32_insert(t, val, NULL);
    }
    printf("Tree Size: %zu\n", tree_int32_size(t));
    printf("Tree Height: %d\n", tree_int32_height(t));
    printf("\n");
    
    void *state;
    key_int32_value_t pair;
    tree_int32_walk_init(t, &state);
    while (state != NULL) {
        pair = tree_int32_walk_next(&state);
        printf("%d ", pair.key);
    }
    printf("\n");

    printf("\n");
    tree_int32_print(t, node_print);
    
    tree_int32_destroy(&t);
        
    return 0;
}
