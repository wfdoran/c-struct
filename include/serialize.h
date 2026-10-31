#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Sizes are written as a base-128 varint: 7 bits per byte, least significant
   group first, high bit set on every byte except the last. */
static inline void serialize_size(size_t x, FILE *fp) {
  do {
    uint8_t v = x & 0x7f;
    x >>= 7;
    if (x != 0) {
      v |= 0x80;
    }
    fputc(v, fp);
  } while (x != 0);
}

/* Returns false on a truncated value or one which does not fit in a size_t;
 *out is set only on success. */
static inline bool deserialize_size(FILE *fp, size_t *out) {
  uint64_t rv = 0;
  for (int shift = 0; shift < 64; shift += 7) {
    int c = fgetc(fp);
    if (c == EOF) {
      return false;
    }
    uint64_t group = (uint64_t) (c & 0x7f);
    if (shift == 63 && group > 1) {
      return false; /* the tenth byte holds only bit 63: more would not fit in
                       64 bits */
    }
    rv |= group << shift;
    if ((c & 0x80) == 0) {
      if (rv > SIZE_MAX) {
        return false;
      }
      *out = (size_t) rv;
      return true;
    }
  }
  return false;
}

/* CRC-32 (IEEE 802.3, the one of zip and PNG).  Start with crc = 0 and pass
   the result back in to checksum data in several pieces. */
static inline uint32_t crc32_update(uint32_t crc, const void *data, size_t n) {
  /* the table for four bits at a time; small, and fast enough here */
  static const uint32_t table[16] = {
      0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
      0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  const uint8_t *p = data;
  crc = ~crc;
  for (size_t i = 0; i < n; i++) {
    crc ^= p[i];
    crc = (crc >> 4) ^ table[crc & 0xf];
    crc = (crc >> 4) ^ table[crc & 0xf];
  }
  return ~crc;
}

/* A checksum is written as four bytes, least significant first, so that files
   do not depend on the byte order of the machine. */
static inline void serialize_u32(uint32_t x, FILE *fp) {
  for (int i = 0; i < 4; i++) {
    fputc((int) ((x >> (8 * i)) & 0xff), fp);
  }
}

static inline bool deserialize_u32(FILE *fp, uint32_t *out) {
  uint32_t rv = 0;
  for (int i = 0; i < 4; i++) {
    int c = fgetc(fp);
    if (c == EOF) {
      return false;
    }
    rv |= (uint32_t) c << (8 * i);
  }
  *out = rv;
  return true;
}

static inline void serialize_string(const char *s, FILE *fp) {
  size_t s_len = strlen(s);
  serialize_size(s_len, fp);
  fwrite(s, sizeof(char), s_len, fp);
}

/* Returns a malloc'ed string, or NULL if the file is truncated, the string is
   longer than max_len, or memory is exhausted.  The length comes from the
   file, so max_len keeps a corrupt file from asking for a huge allocation:
   pass the longest string the caller can accept. */
static inline char *deserialize_string(FILE *fp, size_t max_len) {
  size_t s_len;
  if (!deserialize_size(fp, &s_len) || s_len > max_len || s_len >= SIZE_MAX) {
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
