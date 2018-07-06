#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int
#define prefix int
#include <tree.h>
#undef prefix
#undef data_t

void *update(void *prev, void *curr) {
	if (prev == NULL) {
		int *rv = malloc(sizeof(int));
		*rv = 1;
		return rv;
	}
	
	int *t = (int*) prev;
	int t_val = *t;
	*t = t_val + 1;
	return (void*) t;
}

int main(void) {
    srand(time(NULL));
    
    tree_int_t t;
    tree_int_init(&t);
	tree_int_set_update(&t, &update);
    
    int n = 100;
    
    for (int i = 0; i < n; i++) {
		int v = rand() % n;
		tree_int_insert(&t, v, NULL);
    }
    printf("Tree Size: %d\n", tree_int_size(&t));
    printf("Tree Height: %d\n", tree_int_height(&t));
    printf("\n");
    
    void *state;
    tree_int_walk_init(&t, &state);
    while (state != NULL) {
		key_int_value_t pair = tree_int_walk_next(&state);
		printf("%5d %5d\n", pair.key, *((int *)pair.value));
    }
    printf("\n");
        
    return 0;
}