#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define data_t float
#define prefix float
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    
    tree_float_t *t = tree_float_init();
    
    for (int i = 0; i < 50; i++) {
        float key = (float) rand() / (1 << 15);
        tree_float_insert(t, key, NULL);
    }
    
    while (true) {
        size_t size = tree_float_size(t);
        int height = tree_float_height(t);
        key_float_value_t *kv_pair = tree_float_delete_min(t);
        if (t == NULL) {
            break;
        }
        
        printf("%4d %4d %8.4f\n", size, height, kv_pair->key);
    }
    
    tree_float_destroy(t);
    return 0;
}