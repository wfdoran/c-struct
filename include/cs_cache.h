#ifndef CS_CACHE_H
#define CS_CACHE_H

/* The size in bytes of a cache line.  64 is right for x86 and most ARM
   processors; it can be overridden with -DCS_CACHE_LINE_SIZE=128.  It is only a
   tuning value: nothing depends on it for correctness. */
#ifndef CS_CACHE_LINE_SIZE
#define CS_CACHE_LINE_SIZE (64)
#endif

/* The number of entries of type T an array or queue starts with: as many as fit
   in one cache line, but at least one.  Growing from a single slot would cost
   several allocations for the first few entries. */
#define CS_MIN_CAPACITY(T)                                                     \
  (sizeof(T) >= CS_CACHE_LINE_SIZE ? (size_t) 1                                \
                                   : CS_CACHE_LINE_SIZE / sizeof(T))

#endif
