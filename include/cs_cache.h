/* Shared by array.h, pqueue.h and chan.h.  There is no include guard on
   purpose, as in cmp_hook.h: each of those headers includes this file and
   removes CS_MIN_CAPACITY again at its end, so that it does not leak into the
   includer.  CS_CACHE_LINE_SIZE stays defined, since the user may set it.

   The size in bytes of a cache line.  64 is right for x86 and most ARM
   processors; it can be overridden with -DCS_CACHE_LINE_SIZE=128 (a power of
   two, since chan.h aligns to it).  It is only a tuning value: nothing depends
   on it for correctness. */
#ifndef CS_CACHE_LINE_SIZE
#define CS_CACHE_LINE_SIZE (64)
#endif

/* The number of entries of type T an array or queue starts with: as many as fit
   in one cache line, but at least one.  Growing from a single slot would cost
   several allocations for the first few entries. */
#define CS_MIN_CAPACITY(T)                                                     \
  (sizeof(T) >= CS_CACHE_LINE_SIZE ? (size_t) 1                                \
                                   : CS_CACHE_LINE_SIZE / sizeof(T))
