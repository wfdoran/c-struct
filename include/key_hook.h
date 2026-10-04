/* Internal helper of hash_table.h and phash_table.h, see cmp_hook.h for the
   hooks hkey_hash and hkey_equal.  There is no include guard on purpose; the
   macros are removed again at the end of those headers. */

#ifdef hkey_hash
#define CS_HASH(f, k) hkey_hash(k)
#define CS_HAVE_HASH(f) ((void) (f), 1)
#else
#define CS_HASH(f, k) ((f) (k))
#define CS_HAVE_HASH(f) ((f) != NULL)
#endif

#ifdef hkey_equal
#define CS_KEYEQ(c, x, y) hkey_equal(x, y)
#else
#define CS_KEYEQ(c, x, y) ((c) == NULL || (c) ((x), (y)) == 0)
#endif
