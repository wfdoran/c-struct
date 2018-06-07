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

bool GLUE3(tree_, prefix, _insert)(TREE *a, data_t val) {
    if (a->root == NULL) {
        a->root = GLUE3(tree_, prefix, _init_node(val));
        return true;
    }
    
    NODE *cur = a->root;
    while (true) {
        int c = a->comp(&val, &(cur->key));
        if (c == 0) {
           return false;
        }
        if (c < 0) {
            if (cur->left == NULL) {
                cur->left = GLUE3(tree_, prefix, _init_node(val));
                return true;
            }
            cur = cur->left;
        } else {
            if (cur->right == NULL) {
                cur->right = GLUE3(tree_, prefix, _init_node(val));
                return true;
            }                
            cur = cur->right;
        }
    }
}   

#undef NODE
#undef TREE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
