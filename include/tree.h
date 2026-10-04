// https://algs4.cs.princeton.edu/32bst/

#include <stdlib.h>
#include "cs_assert.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <comp.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define TREE GLUE3(tree_, prefix, _t)
#define NODE GLUE3(tnode_, prefix, _t)
#define KEYVAL GLUE3(key_, prefix, _value_t)
#define TITER GLUE3(titer_, prefix, _t)

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
  int (*comp)(data_t *, data_t *);
  void *(*update)(void *, void *);
  void (*value_free)(void *);
} TREE;

typedef struct {
  data_t key;
  void *value;
  bool found;
} KEYVAL;

/* The position of a walk (see tree_prefix_first): the node which is returned
   next, or NULL at the end. */
typedef struct {
  struct NODE *node;
} TITER;

/* ----------------------------------------------------------------------- */
/*                         constructors                                    */
/* ----------------------------------------------------------------------- */

#define _unused(x) ((void) (x))

/* tree_prefix_t* tree_prefix_init(void);

   allocates and returns an empty binary tree.  The caller must free the
   returned pointer by calling tree_prefix_destroy(&t);
*/
static inline TREE *GLUE3(tree_, prefix, _init)(void) {
  TREE *a = malloc(sizeof(TREE));
  if (a == NULL) {
    return a;
  }

  a->root = NULL;
  data_t temp;
  a->comp = DEFAULT_COMP(temp);
  _unused(temp);
  a->update = NULL;
  a->value_free = NULL;

  _unused(DEFAULT_COMP_TYPE(temp));

  return a;
}

/* void tree_prefix_set_comp(tree_prefix_t *a, int (*comp) (data_t *, data_t *);

   Attaches a comparison function to the binary tree.  For basic
   data_t such as int8_t, int16_t, int32_t, int64_t, float, double,
   char*, this is not needed.  For more complicated data_t, the user
   must define the comparison function.  See comp.h for details and
   the C11 generics which deal with the basic data_ts.
*/
static inline void GLUE3(tree_, prefix,
                         _set_comp)(TREE *a, int (*comp)(data_t *, data_t *)) {
  a->comp = comp;
}

/* void tree_prefix_set_update(tree_prefix_t *a, void *(*update) (void *, void
   *));

   Attaches an update function.  When inserting, if a node with the
   key value already exists, this function is used to update the value
   in that node.

        new_value = update(current_value, passed_value);

    Any memory management must be done by update().  update is also
    used when initializing a node.

        new_value = update(NULL, passed_value)
*/

static inline void
GLUE3(tree_, prefix, _set_update)(TREE *a, void *(*update)(void *, void *) ) {
  a->update = update;
}

/* void tree_prefix_set_value_free(tree_prefix_t *a, void (*value_free)(void *)

    Attaches a free functions for values in each node.  This is used
    in two places:

      1) If no update is provided and tree_prefix_insert hits an
         existing node with the target key, this function is used to
         free to the value at this node before it is replaced by the
         provided value.

      2) When tree_prefix_destroy() is called, this function is used
         to free the value in every node.

    The most common usage to pass the system free() for the value_free
    assuming the values where allocated by the system malloc().
*/

static inline void GLUE3(tree_, prefix,
                         _set_value_free)(TREE *a, void (*value_free)(void *)) {
  a->value_free = value_free;
}

/* ----------------------------------------------------------------------- */
/*                         destructor                                      */
/* ----------------------------------------------------------------------- */

/* tree_prefix_node_destroy(NODE *n, void (*free)(void *));

   Resets to default values and frees a node and all of its children.
   If the user has provided a function for cleaning up the value
   void*, that is applied as well.
*/
static inline void GLUE3(tree_, prefix,
                         _node_destroy)(NODE *n, void (*value_free)(void *)) {
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

   Destroys a tree, frees all of its nodes, applies a user provided
   value_free function to all of the value enties in the nodes, and
   sets the pointer to NULL.
*/

static inline void GLUE3(tree_, prefix, _destroy)(TREE **a_ptr) {
  if (a_ptr == NULL) {
    return;
  }
  TREE *a = *a_ptr;
  // in case a user tries a double destroy
  if (a == NULL) {
    return;
  }
  GLUE3(tree_, prefix, _node_destroy)(a->root, a->value_free);
  a->root = NULL;
  a->comp = NULL;
  a->update = NULL;
  a->value_free = NULL;
  free(a);
  *a_ptr = NULL;
}

/* NODE* tree_prefix_init_node(data_t key);

   Allocates and fills in initial values for a node.
*/

static inline NODE *GLUE3(tree_, prefix, _init_node)(data_t key) {
  NODE *n = malloc(sizeof(NODE));
  if (n == NULL) {
    return NULL;
  }
  n->key = key;
  n->value = NULL;
  n->size = 1;
  n->height = 1;
  n->left = NULL;
  n->right = NULL;
  n->parent = NULL;
  return n;
}

/* void tree_prefix_fillin(NODE *n);

   Fills in the values for size and height for a node assume the
   values for its children are correct.  After inserting or removing a
   node, this is used as you walk up the tree to fix up the values on
   the path.
*/

static inline void GLUE3(tree_, prefix, _fillin)(NODE *n) {
  if (n == NULL) {
    return;
  }
  size_t left_size = n->left == NULL ? 0 : n->left->size;
  size_t right_size = n->right == NULL ? 0 : n->right->size;
  n->size = 1 + left_size + right_size;

  int left_height = n->left == NULL ? 0 : n->left->height;
  int right_height = n->right == NULL ? 0 : n->right->height;
  n->height = 1 + (left_height > right_height ? left_height : right_height);
}

static inline NODE *GLUE3(tree_, prefix, _rotate_left)(NODE *n, bool more);

/* NODE* tree_prefix_rotate_right(NODE *n, bool more);

        n                     m
       / \                   / \
      m   c       =>        a   n
     / \                       / \
    a   b                     b   c

   By recursion, we assume that the height of a and b differ by at
   most one.  Since we are moving a up one level, if ht(a) < ht(b) (=>
   ht(a) = ht(b) - 1), then we want to rotate m first so that ht(a) ==
   ht(b) or ht(a) = ht(b) + 1.  But we don't want to recurse all the
   way done.  The more parameter controls whether we check this
   before applying the rotation.
*/
static inline NODE *GLUE3(tree_, prefix, _rotate_right)(NODE *n, bool more) {
  NODE *m = n->left;
  if (m == NULL) {
    return n;
  }

  if (more) {
    int left_height = m->left == NULL ? 0 : m->left->height;
    int right_height = m->right == NULL ? 0 : m->right->height;
    if (right_height > left_height) {
      m = GLUE3(tree_, prefix, _rotate_left)(m, false);
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

static inline NODE *GLUE3(tree_, prefix, _rotate_left)(NODE *n, bool more) {
  NODE *m = n->right;
  if (m == NULL) {
    return n;
  }

  if (more) {
    int left_height = m->left == NULL ? 0 : m->left->height;
    int right_height = m->right == NULL ? 0 : m->right->height;
    if (left_height > right_height) {
      m = GLUE3(tree_, prefix, _rotate_right)(m, false);
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

/* tree_prefix_balance(NODE *n);

   If the heights of the two children of a node differ by more than 1,
   use either rotate_right or rotate_left to fix this.
*/
static inline NODE *GLUE3(tree_, prefix, _balance)(NODE *n) {
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

/* tree_prefix_insert_node(TREE *a, NODE *n, data_t key, void *value)

   Inserts a key/value at node n in the tree.
*/
/* On an allocation failure, *rc is set to -1 and the subtree is returned
 * unchanged. */
static inline NODE *GLUE3(tree_, prefix, _insert_node)(TREE *a, NODE *n,
                                                       data_t key, void *value,
                                                       int32_t *rc) {
  if (n == NULL) {
    NODE *rv = GLUE3(tree_, prefix, _init_node)(key);
    if (rv == NULL) {
      *rc = -1;
      return NULL;
    }
    if (a->update == NULL) {
      rv->value = value;
    } else {
      rv->value = a->update(rv->value, value);
    }
    return rv;
  }
  int c = a->comp(&key, &(n->key));
  if (c < 0) {
    NODE *child =
        GLUE3(tree_, prefix, _insert_node)(a, n->left, key, value, rc);
    if (*rc != 0) {
      return n;
    }
    n->left = child;
    n->left->parent = n;
  }
  if (c > 0) {
    NODE *child =
        GLUE3(tree_, prefix, _insert_node)(a, n->right, key, value, rc);
    if (*rc != 0) {
      return n;
    }
    n->right = child;
    n->right->parent = n;
  }
  if (c == 0) {
    if (a->update != NULL) {
      n->value = a->update(n->value, value);
    } else {
      if (a->value_free != NULL) {
        a->value_free(n->value);
      }
      n->value = value;
    }
  }

  n = GLUE3(tree_, prefix, _balance)(n);
  GLUE3(tree_, prefix, _fillin)(n);
  return n;
}

/* int32_t tree_prefix_insert(tree_prefix_t *a, data_t key, void *value)

   Inserts a key/value pair into the tree.  The only unclear part is
   what to do if this key already exists in the tree.
     * If the user has provided an update function then

            new_value = update(old_value, passed_value)

     * Otherwise,

            new_value = passed_value

       however if a value_free function has been given, then

            value_free(old_value)

       is done.

   return value:
     -1 => error (a is NULL, no comp function has been set, or out of
           memory; the tree is unchanged)
      0 => ok
*/

static inline int32_t GLUE3(tree_, prefix, _insert)(TREE *a, data_t key,
                                                    void *value) {
  if (a == NULL || a->comp == NULL) {
    return -1;
  }
  int32_t rc = 0;
  NODE *root = GLUE3(tree_, prefix, _insert_node)(a, a->root, key, value, &rc);
  if (rc != 0) {
    return rc;
  }
  a->root = root;
  a->root->parent = NULL;
  return 0;
}

/* NODE* tree_prefix_delete_min_node(NODE *n, NODE **min)

   Removes the minimum node from the non-empty subtree rooted at n.  The
   removed node is returned in min, and the return value is the new root
   of the subtree (rebalanced).
*/
static inline NODE *GLUE3(tree_, prefix, _delete_min_node)(NODE *n,
                                                           NODE **min) {
  if (n->left == NULL) {
    *min = n;
    return n->right;
  }
  n->left = GLUE3(tree_, prefix, _delete_min_node)(n->left, min);
  if (n->left != NULL) {
    n->left->parent = n;
  }
  n = GLUE3(tree_, prefix, _balance)(n);
  GLUE3(tree_, prefix, _fillin)(n);
  return n;
}

/* NODE* tree_prefix_delete_max_node(NODE *n, NODE **max)

   Removes the maximum node from the non-empty subtree rooted at n.  The
   removed node is returned in max, and the return value is the new root
   of the subtree (rebalanced).
*/
static inline NODE *GLUE3(tree_, prefix, _delete_max_node)(NODE *n,
                                                           NODE **max) {
  if (n->right == NULL) {
    *max = n;
    return n->left;
  }
  n->right = GLUE3(tree_, prefix, _delete_max_node)(n->right, max);
  if (n->right != NULL) {
    n->right->parent = n;
  }
  n = GLUE3(tree_, prefix, _balance)(n);
  GLUE3(tree_, prefix, _fillin)(n);
  return n;
}

/* tree_prefix_delete_node(int (*comp) (data_t *, data_t *), NODE *n, data_t
   key, NODE **rv)

   Deletes the node with key value key from the subtree rooted at n.
   The deleted node is returned in rv.  The return value from this
   routine is the new root node for this subtree, which can change due
   to balancing.
*/

static inline NODE *GLUE3(tree_, prefix,
                          _delete_node)(int (*comp)(data_t *, data_t *),
                                        NODE *n, data_t key, NODE **rv) {
  if (n == NULL) {
    *rv = NULL;
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

  /* two children: splice in the in-order successor */
  NODE *m = NULL;
  NODE *new_right = GLUE3(tree_, prefix, _delete_min_node)(n->right, &m);
  m->left = n->left;
  m->right = new_right;
  if (m->left != NULL) {
    m->left->parent = m;
  }
  if (m->right != NULL) {
    m->right->parent = m;
  }
  *rv = n;
  n->left = NULL;
  n->right = NULL;
  n->parent = NULL;
  m = GLUE3(tree_, prefix, _balance)(m);
  GLUE3(tree_, prefix, _fillin)(m);
  return m;
}

/* KEYVAL tree_prefix_delete(TREE *a, data_t key)

   Searches for a node with a given key.  If found the node is removed from the
   tree and the key/value is returned.
*/

static inline KEYVAL GLUE3(tree_, prefix, _delete)(TREE *a, data_t key) {
  if (a->comp == NULL) {
    KEYVAL rv = {.key = key, .value = NULL, .found = false};
    return rv;
  }
  NODE *n = NULL;
  a->root = GLUE3(tree_, prefix, _delete_node)(a->comp, a->root, key, &n);
  if (a->root != NULL) {
    a->root->parent = NULL;
  }

  if (n == NULL) {
    KEYVAL rv = {.key = key, .value = NULL, .found = false};
    return rv;
  }

  KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
  free(n);
  return rv;
}

/*

   Searchs for a node with a given key.  The node is not removed if found.
*/
static inline KEYVAL GLUE3(tree_, prefix, _retrieve)(const TREE *a,
                                                     data_t key) {
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

/*

  Deletes the minimum node based on key.  The key/value of the minimum
  node is returned
*/

static inline KEYVAL GLUE3(tree_, prefix, _delete_min)(TREE *a) {
  if (a->root == NULL) {
    KEYVAL rv = {.value = NULL, .found = false};
    return rv;
  }

  NODE *n = NULL;
  a->root = GLUE3(tree_, prefix, _delete_min_node)(a->root, &n);
  if (a->root != NULL) {
    a->root->parent = NULL;
  }

  KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
  free(n);
  return rv;
}

/*

  Deletes the maximum node based on key.  The key/value of the maximum
  node is returned.
*/
static inline KEYVAL GLUE3(tree_, prefix, _delete_max)(TREE *a) {
  if (a->root == NULL) {
    KEYVAL rv = {.value = NULL, .found = false};
    return rv;
  }

  NODE *n = NULL;
  a->root = GLUE3(tree_, prefix, _delete_max_node)(a->root, &n);
  if (a->root != NULL) {
    a->root->parent = NULL;
  }

  KEYVAL rv = {.key = n->key, .value = n->value, .found = true};
  free(n);
  return rv;
}

/*

  Retrieves the ke/value of the minimum node.
*/
static inline KEYVAL GLUE3(tree_, prefix, _retrieve_min)(const TREE *a) {
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

/*

  Retrieves teh key/value of the maximum node.
*/
static inline KEYVAL GLUE3(tree_, prefix, _retrieve_max)(const TREE *a) {
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

/*

  Returns the number of nodes in the tree.
*/
static inline size_t GLUE3(tree_, prefix, _size)(const TREE *a) {
  return a->root == NULL ? 0 : a->root->size;
}

/*

  Returns the number of nodes with key less than the given value.
*/
static inline size_t GLUE3(tree_, prefix, _num_less)(const TREE *a,
                                                     data_t key) {
  size_t total = 0;
  NODE *n = a->root;
  while (n != NULL) {
    int c = a->comp(&key, &(n->key));

    if (c < 0) {
      n = n->left;
    } else {
      if (c > 0) {
        total++;
      }
      if (n->left != NULL) {
        total += n->left->size;
      }
      n = n->right;
    }
  }
  return total;
}

/*

   Returns the number of nodes with key less than or equal to
   the given value.
*/
static inline size_t GLUE3(tree_, prefix, _num_less_equal)(const TREE *a,
                                                           data_t key) {
  size_t total = 0;
  NODE *n = a->root;
  while (n != NULL) {
    int c = a->comp(&key, &(n->key));

    if (c < 0) {
      n = n->left;
    } else {
      total++;
      if (n->left != NULL) {
        total += n->left->size;
      }
      n = n->right;
    }
  }
  return total;
}

/*

  Returns the number of nodes with key greater than the given value.
*/
static inline size_t GLUE3(tree_, prefix, _num_greater)(const TREE *a,
                                                        data_t key) {
  size_t total = 0;
  NODE *n = a->root;
  while (n != NULL) {
    int c = a->comp(&key, &(n->key));

    if (c > 0) {
      n = n->right;
    } else {
      if (c < 0) {
        total++;
      }
      if (n->right != NULL) {
        total += n->right->size;
      }
      n = n->left;
    }
  }
  return total;
}

/*

  Returns the number of nodes with key greater than or equal to the
  given value.
*/
static inline size_t GLUE3(tree_, prefix, _num_greater_equal)(const TREE *a,
                                                              data_t key) {
  size_t total = 0;
  NODE *n = a->root;
  while (n != NULL) {
    int c = a->comp(&key, &(n->key));

    if (c > 0) {
      n = n->right;
    } else {
      total++;
      if (n->right != NULL) {
        total += n->right->size;
      }
      n = n->left;
    }
  }
  return total;
}

/*

  Returns the height of the tree.
*/
static inline int GLUE3(tree_, prefix, _height)(const TREE *a) {
  return a->root == NULL ? 0 : a->root->height;
}

/* Walks over the tree.  An iterator is a plain variable owned by the caller
   (titer_prefix_t): nothing is allocated and a walk can be stopped at any time.
   The functions return

     0 => *key and *value (either may be NULL) hold the next entry
     1 => there are no more entries

   so a loop over the keys in increasing order is

     titer_prefix_t it;
     for (int32_t rc = tree_prefix_first(t, &it, &key, &value); rc == 0;
          rc = tree_prefix_next(&it, &key, &value)) {
       ...
     }

   The tree must not be changed during a walk.
*/

/* int32_t tree_prefix_next(titer_prefix_t *it, data_t *key, void **value);

   Returns the entry at the iterator and steps the iterator to the next one in
   key order.
*/
static inline int32_t GLUE3(tree_, prefix, _next)(TITER *it, data_t *key,
                                                  void **value) {
  NODE *n = it->node;
  if (n == NULL) {
    return 1;
  }
  if (key != NULL) {
    *key = n->key;
  }
  if (value != NULL) {
    *value = n->value;
  }

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
  it->node = n;
  return 0;
}

/* int32_t tree_prefix_first(const tree_prefix_t *a, titer_prefix_t *it, data_t
   *key, void **value);

   Starts a walk at the smallest key.
*/
static inline int32_t GLUE3(tree_, prefix, _first)(const TREE *a, TITER *it,
                                                   data_t *key, void **value) {
  NODE *n = a->root;
  if (n != NULL) {
    while (n->left != NULL) {
      n = n->left;
    }
  }
  it->node = n;
  return GLUE3(tree_, prefix, _next)(it, key, value);
}

/* int32_t tree_prefix_first_from(const tree_prefix_t *a, data_t from,
   titer_prefix_t *it, data_t *key, void **value);

   Starts a walk at the first node with a key greater than or equal to from, and
   returns 1 if there is none.
*/
static inline int32_t GLUE3(tree_, prefix,
                            _first_from)(const TREE *a, data_t from, TITER *it,
                                         data_t *key, void **value) {
  NODE *best = NULL;
  NODE *n = a->root;
  while (n != NULL) {
    int c = a->comp(&from, &(n->key));
    if (c == 0) {
      best = n;
      break;
    }
    if (c < 0) {
      best = n;
      n = n->left;
    } else {
      n = n->right;
    }
  }
  it->node = best;
  return GLUE3(tree_, prefix, _next)(it, key, value);
}

static inline NODE *GLUE3(tree_, prefix, _post_descent)(NODE *n) {
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

/* int32_t tree_prefix_post_next(titer_prefix_t *it, data_t *key, void **value);
   int32_t tree_prefix_post_first(const tree_prefix_t *a, titer_prefix_t *it,
   data_t *key, void **value);

   The same, but a post-order walk: every node comes after its children, ending
   with the root. This is the order to use to free or destroy things node by
   node.
*/
static inline int32_t GLUE3(tree_, prefix, _post_next)(TITER *it, data_t *key,
                                                       void **value) {
  NODE *n = it->node;
  if (n == NULL) {
    return 1;
  }
  if (key != NULL) {
    *key = n->key;
  }
  if (value != NULL) {
    *value = n->value;
  }

  NODE *p = n->parent;
  if (p != NULL && n == p->left && p->right != NULL) {
    it->node = GLUE3(tree_, prefix, _post_descent)(p->right);
  } else {
    it->node = p;
  }
  return 0;
}

static inline int32_t GLUE3(tree_, prefix, _post_first)(const TREE *a,
                                                        TITER *it, data_t *key,
                                                        void **value) {
  it->node =
      a->root == NULL ? NULL : GLUE3(tree_, prefix, _post_descent)(a->root);
  return GLUE3(tree_, prefix, _post_next)(it, key, value);
}

static inline KEYVAL GLUE3(tree_, prefix, _get_rank)(const TREE *a,
                                                     size_t rank) {
  NODE *n = a->root;
  if (n == NULL || rank >= n->size) {
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

static inline void GLUE3(tree_, prefix,
                         _print_node)(void (*node_print)(data_t, void *),
                                      NODE *n, int depth, uint64_t mask) {
  if (n == NULL) {
    return;
  }

  uint64_t lmask, rmask;
  char s;

  if (n->parent == NULL) {
    lmask = mask;
    rmask = mask;
    s = ' ';
  } else if (n->parent->left == n) {
    lmask = mask;
    rmask = mask | (UINT64_C(1) << depth);
    s = '/';
  } else {
    lmask = mask | (UINT64_C(1) << depth);
    rmask = mask;
    s = '\\';
  }

  GLUE3(tree_, prefix, _print_node)(node_print, n->left, depth + 1, lmask);
  for (int i = 0; i < depth; i++) {
    if ((mask >> i) & 1) {
      printf("|   ");
    } else {
      printf("    ");
    }
  }
  printf("%c-- * ", s);
  if (node_print != NULL) {
    node_print(n->key, n->value);
  }
  printf("\n");
  GLUE3(tree_, prefix, _print_node)(node_print, n->right, depth + 1, rmask);
}

static inline void GLUE3(tree_, prefix,
                         _print)(const TREE *a,
                                 void (*node_print)(data_t, void *)) {
  GLUE3(tree_, prefix, _print_node)(node_print, a->root, 0, 0);
}

#undef NODE
#undef TITER
#undef TREE
#undef GLUE3
#undef GLUE
#undef GLUE_HELPER

/* the internal macros of this header (and its optional null/sentinel setting)
   are removed so that they do not leak into the includer */
#undef _unused
#undef KEYVAL
