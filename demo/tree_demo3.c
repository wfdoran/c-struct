#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t double
#define prefix double
#include <tree.h>
#undef prefix
#undef data_t


void walk(tnode_double_t *n) {
    if (n == NULL) {
        return;
    }
    walk(n->left);
    for (int i = 0; i < n->height; i++) {
        printf("  ");
    }
    printf("%.4f", n->key);
    if (n->parent != NULL) {
        printf("              %.4f", n->parent->key);
    }
    printf("\n");
    walk(n->right);
}

int main(void) {
    srand(time(NULL));
    
    tree_double_t *t = tree_double_init();
    
    int n = 100;
    double vals[n];
    
    for (int i = 0; i < n; i++) {
        vals[i] = ((double) ((rand() >> 3) & 0xffff)) / 0x1000;
        // printf("IN %.4f\n", vals[i]);
        tree_double_insert(t, vals[i], NULL);
    }
    printf("Tree Size: %d\n", tree_double_size(t));
    printf("Tree Height: %d\n", tree_double_height(t));
    printf("\n");
    
    void *state;
    tree_double_walk_init(t, &state);
    while (state != NULL) {
		key_double_value_t pair = tree_double_walk_next(&state);
        printf("%.4f ", pair.key);
    }
    printf("\n");
        
    for (int i = 0; i < 2 * n; i++) {
        int j = rand() % n;
        int k = rand() % n;
        double temp = vals[k];
        vals[k] = vals[j];
        vals[j] = temp;
    }
    
    //walk(t.root);
    
    for (int i = 0; i < n/2; i++) {
        // printf("OUT %.4f\n", vals[i]);
        tree_double_delete(t, vals[i]);
    }
    printf("\n");
    
    // walk(t.root);
    printf("Tree Size: %d\n", tree_double_size(t));
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