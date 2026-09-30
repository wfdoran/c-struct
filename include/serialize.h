#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Sizes are written as a base-128 varint: 7 bits per byte, least significant
   group first, high bit set on every byte except the last. */
static void serialize_size(size_t x, FILE *fp) {
  do {
    uint8_t v = x & 0x7f;
    x >>= 7;
    if (x != 0) {
      v |= 0x80;
    }
    fputc(v, fp);
  } while (x != 0);
}

/* Returns false on a truncated or overlong value; *out is set only on success. */
static bool deserialize_size(FILE *fp, size_t *out) {
  size_t rv = 0;
  for (int shift = 0; shift < 64; shift += 7) {
    int c = fgetc(fp);
    if (c == EOF) {
      return false;
    }
    rv |= (size_t) (c & 0x7f) << shift;
    if ((c & 0x80) == 0) {
      *out = rv;
      return true;
    }
  }
  return false;
}

static void serialize_string(const char *s, FILE *fp) {
  size_t s_len = strlen(s);
  serialize_size(s_len, fp);
  fwrite(s, sizeof(char), s_len, fp);
}

static char* deserialize_string(FILE *fp) {
  size_t s_len;
  if (!deserialize_size(fp, &s_len) || s_len >= SIZE_MAX) {
    return NULL;
  }
  char *s = malloc(s_len + 1);
  if (s == NULL) {
    return NULL;
  }
  if (fread(s, sizeof(char), s_len, fp) != s_len) {
    free(s);
    return NULL;
  }
  s[s_len] = 0;
  return s;
}

#endif
