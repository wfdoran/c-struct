#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    
    tree_int32_t *t = tree_int32_init();
    
    for (int i = 0; i < 100; i++) {
        int32_t val = (rand() >> 3) & 0xffff;
        tree_int32_insert(t, val, NULL);
    }
    printf("Tree Size: %zu\n", tree_int32_size(t));
    printf("Tree Height: %d\n", tree_int32_height(t));
    printf("\n");
    
    void *state = NULL;
    tree_int32_postwalk_init(t, &state);
    while (state != NULL) {
        key_int32_value_t pair = tree_int32_postwalk_next(&state);
        printf("%d ", pair.key);
    }
    printf("\n");
    
    tree_int32_destroy(&t);
        
    return 0;
}
