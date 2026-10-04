#ifndef CS_ASSERT_H
#define CS_ASSERT_H

#include <stdio.h>
#include <stdlib.h>

/* CS_ASSERT(cond): if cond is false, prints "ASSERT failure: file function line
   condition" to stderr and aborts.  Unlike assert() from <assert.h>, it is
   always active; -DNDEBUG does not remove it, so a misuse the containers
   document as an assert failure is caught in release builds too. */
static inline _Noreturn void cs_assert_fail(const char *file, const char *func,
                                            int line, const char *cond) {
  fprintf(stderr, "ASSERT failure: %s %s %d %s\n", file, func, line, cond);
  abort();
}

#define CS_ASSERT(cond)                                                        \
  do {                                                                         \
    if (!(cond)) {                                                             \
      cs_assert_fail(__FILE__, __func__, __LINE__, #cond);                     \
    }                                                                          \
  } while (0)

#endif
