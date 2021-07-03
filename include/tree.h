// https://algs4.cs.princeton.edu/32bst/

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

// rank
// move routines which the caller should not see to <tree_private.h> 
// initialize iterator with rank or bound or something

// search and replace

/* ----------------------------------------------------------------------- */
/*                         data structures                                 */
/* ----------------------------------------------------------------------- */
                              
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
    void (*value_free) (void *);
} TREE;

typedef struct {
    data_t key;
    void *value;
    bool found;
} KEYVAL;

/* ----------------------------------------------------------------------- */
/*                         constructors                                    */
/* ----------------------------------------------------------------------- */

/* tree_prefix_t* tree_prefix_init(); 

   allocates and returns an empty binary tree.  The caller must free the 
   returned pointer by calling tree_prefix_destroy(t);   
*/
TREE *GLUE3(tree_, prefix, _init) () {
    TREE *a = malloc(sizeof(TREE));
    if (a == NULL) {
        return a;
    }
    
    a->root = NULL;
    data_t temp;
    a->comp = DEFAULT_COMP(temp);
    a->update = NULL;
    a->value_free = NULL;
    
    return a;
}


/* void tree_prefix_set_comp(tree_prefix_t *a, int (*comp) (data_t *, data_t *);

   Attaches a comparison function to the binary tree.  For basic data_t such as 
   int8_t, int16_t, int32_t, int64_t, float, double, char*, this is not needed.
   For more complicated data_t, the user must define the comparison function.
   See comp.h for details and the C11 generics which deal with the basic data_ts.
*/
void GLUE3(tree_, prefix, _set_comp) (TREE *a, int (*comp) (data_t *, data_t *)) {
    a->comp = comp;
}

/* void tree_prefix_set_update(tree_prefix_t *a, void *(*update) (void *, void *));

   Attaches an update function.  When inserting, if node the key value already 
   exists, this function is used to update the value in that node.  
   
        new_value = update(current_value, passed_value);
        
    Any memory management must be done by update().  update is also used when 
    initializing a node.
    
        new_value = update(NULL, passed_value)   
*/

void GLUE3(tree_, prefix, _set_update) (TREE *a, void *(*update) (void *, void *)) {
    a->update = update;
}

/* void tree_prefix_set_value_free(tree_prefix_t *a, void (*value_free)(void *)

    Attaches a free functions for values in each node.  This is used in two places:
      1) If no update is provided and tree_prefix_insert hits an existing node
         with the target key, this function is used to free to the value at
         this node before it is replaced by the provided value.
      2) When tree_prefix_destroy() is called, this function is used to 
         free the value in every node.
  
    The most common usage to pass the system free() for the value_free assuming
    the values where allocated by the system malloc().    
*/

void GLUE3(tree_, prefix, _set_value_free) (TREE *a, void (*value_free)(void *)) {
    a->value_free = value_free;
}    


/* ----------------------------------------------------------------------- */
/*                         destructor                                      */
/* ----------------------------------------------------------------------- */

static void GLUE3(tree_, prefix, _node_destroy) (NODE *n, void (*value_free)(void *)) {
    if (n == NULL) {
        return;
    }
    
    GLUE3(tree_, prefix, _node_destroy)(n->left, value_free);
    GLUE3(tree_, prefix, _node_destroy)(n->right, value_free);
    n->left = NULL;
    n->right = NULL;
    n->parent = NULL;
    
    if (value_free != NULL) {
        value_free(n->value);
    }
    free(n);
}

/* void tree_prefix_destroy(tree_prefix_t **a)

   Destroys a tree, frees all of its nodes, and sets the pointer to NULL.        
*/   

void GLUE3(tree_, prefix, _destroy) (TREE **a_ptr) {
    TREE *a = *a_ptr;
    // in case a user tries a double destroy
    if (a == NULL) { 
        return;
    }
    GLUE3(tree_, prefix, _node_destroy) (a->root, a->value_free);
    a->root = NULL;
    a->comp = NULL;
    a->update = NULL;
    a->value_free = NULL;
    free(a);
    a_ptr = NULL;
}

NODE* GLUE3(tree_, prefix, _init_node)(data_t key) {
    NODE *n = malloc(sizeof(NODE));
    n->key = key;
    n->value = NULL;
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

NODE* GLUE3(tree_, prefix, _insert_node)(TREE *a, NODE *n, data_t key, void *value) {
    if (n == NULL) {
        NODE *rv = GLUE3(tree_, prefix, _init_node)(key);
        if (a->update == NULL) {
            rv->value = value;
        } else {
            rv->value = a->update(rv->value, value);
        }
        return rv;
    }
    int c = a->comp(&key, &(n->key));
    if (c < 0) {
        n->left = GLUE3(tree_, prefix, _insert_node)(a, n->left, key, value);
        n->left->parent = n;
    }
    if (c > 0) {
        n->right = GLUE3(tree_, prefix, _insert_node)(a, n->right, key, value);
        n->right->parent = n;        
    }
    if (c == 0) {
        if (a->update == NULL) {
            if (a->value_free != NULL) {
                a->value_free(n->value);
            }
            n->value = value;
        } else {
            n->value = a->update(n->value, value);
        }
    }
    
    n = GLUE3(tree_, prefix, _balance)(n);
    GLUE3(tree_, prefix, _fillin)(n);
    return n;
}

/* void tree_prefix_insert(tree_prefix_t *a, data_t key, void *value)

   Inserts a key/value pair into the tree.  The only unclear part is
   what to do if this key already exists in the tree.  
     * If the user has provided an update function then 
     
            new_value = update(old_value, passed_value)
            
     * Otherwise,

            new_value = passed_value

       however if a value_free function has been given, then 

            value_free(old_value) 

       is done. 
*/       

void GLUE3(tree_, prefix, _insert)(TREE *a, data_t key, void *value) {
    a->root = GLUE3(tree_, prefix, _insert_node)(a, a->root, key, value);
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

KEYVAL GLUE3(tree_, prefix, _retrieve)(TREE *a, data_t key) {
    NODE *n = a->root;
    
    while (true) {
        if (n == NULL) {
            KEYVAL rv = {.key = key, .value = NULL, .found = false};
            return rv;
        }
        
        int c = a->comp(&key, &(n->key));
        if (c == 0) {
            KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
            return rv;
        }
        
        n = c < 0 ? n->left : n->right; 
    }
}

KEYVAL GLUE3(tree_, prefix, _delete)(TREE *a, data_t key) {
    NODE *n = NULL;
    a->root = GLUE3(tree_, prefix, _delete_node)(a->comp, a->root, key, &n);
    if (a->root != NULL) {
        a->root->parent = NULL;
    }
    
    if (n == NULL) {
        KEYVAL rv = {.key = key, .value = NULL, .found = false};
        return rv;
    }
    
    KEYVAL rv = {.key = key, .value = n->value, .found = true};
    free(n);
    return rv;
}

KEYVAL GLUE3(tree_, prefix, _delete_min)(TREE *a) {
    NODE *n = a->root;
    if (n == NULL) {
        KEYVAL rv = {.value = NULL, .found = false};
        return rv;
    }
            
    while (n->left != NULL) {
        n = n->left;
    }
    
    return GLUE3(tree_, prefix, _delete)(a, n->key);
}    

KEYVAL GLUE3(tree_, prefix, _delete_max)(TREE *a) {
    NODE *n = a->root;
    if (n == NULL) {
        KEYVAL rv = {.value = NULL, .found = false};
        return rv;
    }
            
    while (n->right != NULL) {
        n = n->right;
    }
    
    return GLUE3(tree_, prefix, _delete)(a, n->key);
}    
    
KEYVAL GLUE3(tree_, prefix, _retrieve_min)(TREE *a) {
    NODE *n = a->root;
    
    if (n == NULL) {
        KEYVAL rv = {.value = NULL, .found = false};
        return rv;
    }
    
    while (n->left != NULL) {
        n = n->left;
    }

    KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
    return rv;
}    

KEYVAL GLUE3(tree_, prefix, _retrieve_max)(TREE *a) {
    NODE *n = a->root;
    
    if (n == NULL) {
        KEYVAL rv = {.value = NULL, .found = false};
        return rv;
    }
    
    while (n->right != NULL) {
        n = n->right;
    }

    KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
    return rv;
}


size_t GLUE3(tree_, prefix, _size)(TREE *a) {
    return a->root == NULL ? 0 : a->root->size;
}

int GLUE3(tree_, prefix, _height)(TREE *a) {
    return a->root == NULL ? 0 : a->root->height; 
}

NODE *GLUE3(tree_, prefix, _postwalk_descent)(NODE *n) {
    while (true) {
        if (n->left != NULL) {
            n = n->left;
            continue;
        }
        
        if (n->right != NULL) {
            n = n->right;
            continue;
        }
        
        return n;
    }
}
    
void GLUE3(tree_, prefix, _postwalk_init)(TREE *a, void **state) {
    if (a->root == NULL) {
        *state = NULL;
    } else {
        NODE *n = GLUE3(tree_, prefix, _postwalk_descent)(a->root);
        *state = n;
    }
}

KEYVAL GLUE3(tree_, prefix, _postwalk_next)(void **state) {
    NODE *n = *state;
    KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
    
    NODE *p = n->parent;
    if (p != NULL && n == p->left && p->right != NULL) {
        *state = GLUE3(tree_, prefix, _postwalk_descent)(p->right);
    } else {
        *state = p;
    }
    return rv;
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
    KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
    
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

KEYVAL GLUE3(tree_, prefix, _get_rank)(TREE *a, size_t rank) {
    NODE *n = a->root;
    if (rank < 0 || rank >= n->size) {
        KEYVAL rv = {.value = NULL, .found = false};
        return rv;
    }
    
    while (true) {
        size_t left_size = n->left == NULL ? 0 : n->left->size;
        if (rank < left_size) {
            n = n->left;
            continue;
        }
        if (rank > left_size) {
            rank -= left_size + 1;
            n = n->right;
            continue;
        }
                
        KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
        return rv;
    }
}

#undef NODE
#undef TREE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
