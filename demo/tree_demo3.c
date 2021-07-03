#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t double
#define prefix double
#include <tree.h>
#undef prefix
#undef data_t


int main(void) {
    srand(time(NULL));
    
    tree_double_t *t = tree_double_init();
    
    int n = 100;
    double vals[n];
    
    for (int i = 0; i < n; i++) {
        vals[i] = ((double) ((rand() >> 3) & 0xffff)) / 0x1000;
        tree_double_insert(t, vals[i], NULL);
    }
    printf("Tree Size: %zu\n", tree_double_size(t));
    printf("Tree Height: %d\n", tree_double_height(t));
    printf("\n");
    
    void *state;
    tree_double_walk_init(t, &state);
    while (state != NULL) {
	key_double_value_t pair = tree_double_walk_next(&state);
        printf("%.4f ", pair.key);
    }
    printf("\n");
        
    for (int i = 0; i < n/2; i++) {
        tree_double_delete(t, vals[i]);
    }
    printf("\n");
    
    printf("Tree Size: %zu\n", tree_double_size(t));
    printf("Tree Height: %d\n", tree_double_height(t));
    printf("\n");
    
    tree_double_walk_init(t, &state);
    while (state != NULL) {
        key_double_value_t pair = tree_double_walk_next(&state);
        printf("%.4f ", pair.key);
    }
    printf("\n");
        
    tree_double_destroy(&t);    
        
    return 0;
}
