/* Internal helper of array.h, tree.h, pqueue.h and linked_list.h (hash_table.h
   and phash_table.h use key_hook.h); not meant to be included by users.  There
   is no include guard on purpose: each of those headers includes this file and
   removes the macros again at its end.

   By default, the ordering of data_t, and the hash and equality of hkey_t, are
   called through the function pointers stored in the container.  A user who
   wants the compiler to inline them defines, before including the header,

     #define data_less(x, y) ((x) < (y))     array, tree, pqueue, linked_list
     #define hkey_hash(k) ...                hash_table, phash_table
     #define hkey_equal(x, y) ((x) == (y))   hash_table, phash_table

   data_less takes two data_t values and must be a strict weak ordering.
   hkey_hash takes an hkey_t and returns a uint64_t.  hkey_equal takes two
   hkey_t and is true when they are the same key.  The arguments of these
   macros are evaluated more than once, so they must not have side effects.

   Once a hook is defined, the corresponding function pointer (set_comp,
   set_hash) is no longer used.  The hooks are optional and independent.  The
   macros are removed at the end of the container header.
*/

#ifdef data_less
/* three way comparison made of the user's less-than; the compiler reduces the
   common tests (< 0, <= 0, > 0) to a single use of data_less */
#define CS_CMP(c, x, y)                                                        \
  (data_less(*(x), *(y)) ? -1 : (data_less(*(y), *(x)) ? 1 : 0))
#define CS_HAVE_CMP(c) ((void) (c), 1)
#define CS_ASSERT_CMP(c) ((void) (c))
#else
#define CS_CMP(c, x, y) ((c) ((x), (y)))
#define CS_HAVE_CMP(c) ((c) != NULL)
#define CS_ASSERT_CMP(c) CS_ASSERT(c != NULL)
#endif
