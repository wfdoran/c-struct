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
#define KEYVAL GLUE3(key_, prefix, _value_t)

typedef struct NODE {
    data_t key;
	void *value;
    size_t size;
    int height;
    struct NODE *left;
    struct NODE *right;
    struct NODE *parent;
} NODE;

typedef struct {
    struct NODE *root;
    int (*comp) (data_t *, data_t *);
	void * (*update) (void *, void *);
} TREE;

typedef struct {
	data_t key;
	void *value;
} KEYVAL;

void GLUE3(tree_, prefix, _init) (TREE *a) {
    a->root = NULL;
    data_t temp;
    a->comp = DEFAULT_COMP(temp);
	a->update = NULL;
}

void GLUE3(tree_, prefix, _set_comp) (TREE *a, int (*comp) (data_t *, data_t *)) {
	a->comp = comp;
}

void GLUE3(tree_, prefix, _set_update) (TREE *a, void *(*update) (void *, void *)) {
	a->update = update;
}

NODE* GLUE3(tree_, prefix, _init_node)(data_t key, void *value) {
    NODE *n = malloc(sizeof(NODE));
    n->key = key;
	n->value = value;
    n->size = 1;
    n->height = 1;
    n->left = NULL;
    n->right = NULL;
    n->parent = NULL;
    return n;
}

void GLUE3(tree_, prefix, _fillin)(NODE *n) {
    if (n == NULL) {
        return;
    }
    size_t left_size = n->left == NULL ? 0 : n->left->size;
    size_t right_size = n->right == NULL ? 0 : n->right->size;
    n->size = 1 + left_size + right_size;
    
    int left_height = n->left == NULL ? 0 : n->left->height;
    int right_height = n->right == NULL ? 0 : n->right->height;
    n->height =  1 + (left_height > right_height ? left_height : right_height);
    
}

NODE* GLUE3(tree_, prefix, _rotate_left) (NODE *n, bool more);

NODE* GLUE3(tree_, prefix, _rotate_right) (NODE *n, bool more) {
    NODE *m = n->left;
    if (m == NULL) {
        return n;
    }
    
    if (more) {
        int left_height = m->left == NULL ? 0 : m->left->height;
        int right_height = m->right == NULL ? 0 : m->right->height;
        if (right_height > left_height) {
            m = GLUE3(tree_, prefix, _rotate_left) (m, false);
        }
    }
    
    NODE *a = m->left;
    NODE *b = m->right;
    NODE *c = n->right;
    
    n->left = b;
    n->right = c;
    n->parent = m;
    GLUE3(tree_, prefix, _fillin)(n);
    if (b != NULL) {
        b->parent = n;
    }
    
    m->left = a;
    m->right = n;
    m->parent = NULL;
        
    return m;
}

NODE* GLUE3(tree_, prefix, _rotate_left) (NODE *n, bool more) {
    NODE *m = n->right;
    if (m == NULL) {
        return n;
    }
    
    if (more) {
        int left_height = m->left == NULL ? 0 : m->left->height;
        int right_height = m->right == NULL ? 0 : m->right->height;
        if (left_height > right_height) {
            m = GLUE3(tree_, prefix, _rotate_right) (m, false);
        }
    }

    NODE *a = n->left;
    NODE *b = m->left;
    NODE *c = m->right;
    
    n->left = a;
    n->right = b;
    n->parent = m;
    GLUE3(tree_, prefix, _fillin)(n);
    if (b != NULL) {
        b->parent = n;
    }
    
    m->left = n;
    m->right = c;
    m->parent = NULL;
    return m;
}

NODE* GLUE3(tree_, prefix, _balance) (NODE *n) {
    int left_height = n->left == NULL ? 0 : n->left->height;
    int right_height = n->right == NULL ? 0 : n->right->height;
    
    if (left_height >= right_height + 2) {
        return GLUE3(tree_, prefix, _rotate_right)(n, true);
    }
    if (right_height >= left_height + 2) {
        return GLUE3(tree_, prefix, _rotate_left)(n, true);
    }
    return n;   
}

NODE* GLUE3(tree_, prefix, _insert_node)(int (*comp) (data_t *, data_t *), void * (*update) (void *, void *), NODE *n, data_t key, void *value) {
    if (n == NULL) {
        return GLUE3(tree_, prefix, _init_node)(key, value);
    }
    int c = comp(&key, &(n->key));
    if (c < 0) {
        n->left = GLUE3(tree_, prefix, _insert_node)(comp, update, n->left, key, value);
        n->left->parent = n;
    }
    if (c > 0) {
        n->right = GLUE3(tree_, prefix, _insert_node)(comp, update, n->right, key, value);
        n->right->parent = n;        
    }
	if (c == 0) {
		if (n->value == NULL || update == NULL) {
			n->value = value;
		} else {
			n->value = update(n->value, value);
		}
	}
    
    n = GLUE3(tree_, prefix, _balance)(n);
    GLUE3(tree_, prefix, _fillin)(n);
    return n;
}

void GLUE3(tree_, prefix, _insert)(TREE *a, data_t key, void *value) {
    a->root = GLUE3(tree_, prefix, _insert_node)(a->comp, a->update, a->root, key, value);
    a->root->parent = NULL;
}

NODE* GLUE3(tree_, prefix, _delete_node)(int (*comp) (data_t *, data_t *), NODE *n, data_t key, NODE **rv) {
    if (n == NULL) {
       *rv == NULL;
       return NULL;
    }
    
    int c = comp(&key, &(n->key));
    if (c != 0) {
        if (c < 0) {
            n->left = GLUE3(tree_, prefix, _delete_node)(comp, n->left, key, rv);
            if (n->left != NULL) {
                n->left->parent = n;
            }
        }
        if (c > 0) {
            n->right = GLUE3(tree_, prefix, _delete_node)(comp, n->right, key, rv);
            if (n->right != NULL) {
                n->right->parent = n;
            }
        }
        n = GLUE3(tree_, prefix, _balance)(n);
        GLUE3(tree_, prefix, _fillin)(n);
        return n;
    }
    
    if (n->left == NULL) {
        *rv = n;
        return n->right;
    }
    
    if (n->right == NULL) {
        *rv = n;
        return n->left;
    }
        
    n = GLUE3(tree_, prefix, _rotate_right)(n, false);
    return GLUE3(tree_, prefix, _delete_node)(comp, n, key, rv);
}

void* GLUE3(tree_, prefix, _delete)(TREE *a, data_t val) {
    NODE *n = NULL;
    a->root = GLUE3(tree_, prefix, _delete_node)(a->comp, a->root, val, &n);
	void *rv = NULL;
	if (n != NULL) {
		rv = n->value;
	}
    free(n);
    if (a->root != NULL) {
        a->root->parent = NULL;
    }
	return rv;
}    

size_t GLUE3(tree_, prefix, _size)(TREE *a) {
    return a->root == NULL ? 0 : a->root->size;
}

int GLUE3(tree_, prefix, _height)(TREE *a) {
    return a->root == NULL ? 0 : a->root->height;   
}

void GLUE3(tree_, prefix, _walk_init)(TREE *a, void **state) {
    if (a->root == NULL) {
        *state = NULL;
    } else {
        NODE *n = a->root;
        while (n->left != NULL) {
            n = n->left;
        }
        *state = n;
    }
}

KEYVAL GLUE3(tree_, prefix, _walk_next)(void **state) {
    NODE *n = *state;
    KEYVAL rv = {.key = n->key, .value = n->value};
    
    if (n->right != NULL) {
        n = n->right;
        while (n->left != NULL) {
            n = n->left;
        }
    } else {
        while (true) {
            NODE *prev = n;
            n = n->parent;
            if (n == NULL || n->left == prev) {
                break;
            }        
        }
    }
    *state = n;
    return rv;
}

#undef NODE
#undef TREE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
