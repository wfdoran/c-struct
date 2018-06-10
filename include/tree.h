#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <comp.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define TREE GLUE3(tree_, prefix, _t)
#define NODE GLUE3(tnode_, prefix, _t)

typedef struct NODE {
    data_t key;
    struct NODE *left;
    struct NODE *right;
} NODE;

typedef struct {
    struct NODE *root;
    int (*comp) (data_t *, data_t *);
} TREE;

void GLUE3(tree_, prefix, _init) (TREE *a) {
    a->root = NULL;
    data_t temp;
    a->comp = DEFAULT_COMP(temp);
}

NODE* GLUE3(tree_, prefix, _init_node)(data_t val) {
    NODE *n = malloc(sizeof(NODE));
    n->key = val;
    n->left = NULL;
    n->right = NULL;
    return n;
}

NODE* GLUE3(tree_, prefix, _insert_node)(int (*comp) (data_t *, data_t *), NODE *n, data_t val) {
    if (n == NULL) {
        return GLUE3(tree_, prefix, _init_node)(val);
    }
    int c = comp(&val, &(n->key));
    if (c < 0) {
        n->left = GLUE3(tree_, prefix, _insert_node)(comp, n->left, val);        
    }
    if (c > 0) {
        n->right = GLUE3(tree_, prefix, _insert_node)(comp, n->right, val);   
    }
    return n;
}

void GLUE3(tree_, prefix, _insert)(TREE *a, data_t val) {
    a->root = GLUE3(tree_, prefix, _insert_node)(a->comp, a->root, val);
}

#undef NODE
#undef TREE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
