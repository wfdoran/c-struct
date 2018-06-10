#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

void walk_tree(tnode_int32_t *n) {
    if (n->left != NULL) {
        walk_tree(n->left);
    }
    printf("%d\n", n->key);
    if (n->right != NULL) {
        walk_tree(n->right);
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
    walk_tree(t.root);
    return 0;
}