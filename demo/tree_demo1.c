#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

void walk_tree(tnode_int32_t *n, int depth) {
    if (n->left != NULL) {
        walk_tree(n->left, depth+1);
    }
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("%d\n", n->key);
    if (n->right != NULL) {
        walk_tree(n->right, depth+1);
    }        
}

int main(void) {
    srand(time(NULL));
    
    tree_int32_t t;
    tree_int32_init(&t);
    
    for (int i = 0; i < 10; i++) {
        int32_t val = (rand() >> 3) & 0xf;
        tree_int32_insert(&t, val);
    }
    printf("Tree Size: %d\n", tree_int32_size(&t));
    // walk_tree(t.root, 0);
    
    void *state;
    tree_int32_walk_init(&t, &state);
    int count = 0;
    while (state != NULL) {
        printf("%d\n", tree_int32_walk_next(&state));
        count++;
        if (count > 12) {
            break;
        }
    }
        
    return 0;
}