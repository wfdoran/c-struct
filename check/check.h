#include <stdio.h>

#define START_TEST(a)				\
  int a() {					\
    {

#define END_TEST				\
  }						\
  printf("%s passed\n", __func__);		\
  return 0;					\
}

#define CHECK(a)				\
  do {						\
  if (!(a)) {					\
  printf("%s failed: %s\n", __func__, #a);	\
  return 1;					\
  } \
  } while(0)
