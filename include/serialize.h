#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include <stdbool.h>

static void serialize_size(size_t x, FILE *fp) {
  size_t tmp = x;
  while (tmp > 0) {
    uint8_t v = x & 0x7f;
    tmp >>= 7;
    if (tmp > 0) {
      v ^= 0x80;
    }
    fputc(v, fp);
  }
}

static size_t deserialize_size(FILE *fp) {
  size_t rv = 0;
  while (true) {
    int c = fgetc(fp);
    if (c == EOF) {
      return rv;
    }

    rv = (rv << 7) ^ (c & 0x7f);
    if ((c & 0x80) == 0) {
      return rv;
    }
  }
}

static void serialize_string(const char *s, FILE *fp) {
  size_t s_len = strlen(s);
  serialize_size(s_len, fp);
  fwrite(s, sizeof(char), s_len, fp);
}

static char* deserialize_string(FILE *fp) {
  size_t s_len = deserialize_size(fp);
  char *s = malloc((s_len + 1) * sizeof(char));
  if (s == NULL) {
    return NULL;
  }
  size_t num_read = fread(s, sizeof(char), s_len, fp);
  if (num_read != s_len) {
    return NULL;
  }
  s[s_len] = 0;
  return s;
}

#endif
