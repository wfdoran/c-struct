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
    int32_t num_items = 100;
    
    tree_int32_t *t = tree_int32_init();
    
    for (int i = 0; i < num_items; i++) {
        int32_t val = (rand() >> 3) & 0xffff;
        tree_int32_insert(t, val, NULL);
    }
    printf("Tree Size: %zu\n", tree_int32_size(t));
    printf("Tree Height: %d\n", tree_int32_height(t));
    printf("\n");
    
    titer_int32_t it;
    int32_t key;
    int32_t left = 0x3fff;
    int32_t right = 0xcfff;

    printf("From %d to %d\n", left, right);
    
    tree_int32_insert(t, left - 1, NULL);
    tree_int32_insert(t, left, NULL);   
    tree_int32_insert(t, left + 1, NULL);
    tree_int32_insert(t, right - 1, NULL);
    tree_int32_insert(t, right, NULL);   
    tree_int32_insert(t, right + 1, NULL);
   
    
    for (int32_t rc = tree_int32_first_from(t, left, &it, &key, NULL); rc == 0; rc = tree_int32_next(&it, &key, NULL)) {
	if (key > right) {
	  break;
	}
        printf("%d ", key);
    }
    printf("\n");
    
    tree_int32_destroy(&t);
        
    return 0;
}
