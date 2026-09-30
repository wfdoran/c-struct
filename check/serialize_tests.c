#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define data_t int
#define prefix ser
#include <array.h>
#undef data_t
#undef prefix

#define data_t double
#define prefix serd
#include <array.h>
#undef data_t
#undef prefix

#define TEST_FILE "serialize_test.bin"

static void write_bytes(const char *filename, const char *buf, long n) {
  FILE *fp = fopen(filename, "wb");
  fwrite(buf, 1, n, fp);
  fclose(fp);
}

static long read_bytes(const char *filename, char *buf, long max) {
  FILE *fp = fopen(filename, "rb");
  long n = fread(buf, 1, max, fp);
  fclose(fp);
  return n;
}

START_TEST(serialize_test1)

size_t values[] = {0, 1, 127, 128, 129, 300, 16383, 16384, 1000000,
                   (size_t) 1 << 32, (size_t) 1 << 62, SIZE_MAX};

for (int i = 0; i < (int) (sizeof(values) / sizeof(values[0])); i++) {
  FILE *fp = fopen(TEST_FILE, "wb");
  serialize_size(values[i], fp);
  fputc(0xab, fp);
  fclose(fp);

  fp = fopen(TEST_FILE, "rb");
  size_t r = 12345;
  CHECK(deserialize_size(fp, &r));
  CHECK(r == values[i]);
  CHECK(fgetc(fp) == 0xab);
  fclose(fp);
}

remove(TEST_FILE);

END_TEST

START_TEST(serialize_test2)

// small values take one byte, and larger ones grow by one byte per 7 bits
size_t sizes[] = {0, 127, 128, 16383, 16384};
long bytes[] = {1, 1, 2, 2, 3};
for (int i = 0; i < 5; i++) {
  FILE *fp = fopen(TEST_FILE, "wb");
  serialize_size(sizes[i], fp);
  fclose(fp);
  char buf[16];
  CHECK(read_bytes(TEST_FILE, buf, sizeof(buf)) == bytes[i]);
}

// a truncated value and an overlong one are rejected without setting *out
write_bytes(TEST_FILE, "\x80", 1);
FILE *fp = fopen(TEST_FILE, "rb");
size_t r = 7;
CHECK(!deserialize_size(fp, &r));
CHECK(r == 7);
fclose(fp);

write_bytes(TEST_FILE, "", 0);
fp = fopen(TEST_FILE, "rb");
CHECK(!deserialize_size(fp, &r));
fclose(fp);

char overlong[12];
memset(overlong, 0xff, sizeof(overlong));
write_bytes(TEST_FILE, overlong, sizeof(overlong));
fp = fopen(TEST_FILE, "rb");
CHECK(!deserialize_size(fp, &r));
CHECK(r == 7);
fclose(fp);

remove(TEST_FILE);

END_TEST

START_TEST(serialize_test3)

char *long_string = malloc(1001);
memset(long_string, 'x', 1000);
long_string[1000] = 0;

const char *strs[] = {"", "hi", "a", long_string};
FILE *fp = fopen(TEST_FILE, "wb");
for (int i = 0; i < 4; i++) {
  serialize_string(strs[i], fp);
}
fclose(fp);

fp = fopen(TEST_FILE, "rb");
for (int i = 0; i < 4; i++) {
  char *s = deserialize_string(fp);
  CHECK(s != NULL);
  CHECK(strcmp(s, strs[i]) == 0);
  free(s);
}
CHECK(deserialize_string(fp) == NULL);
fclose(fp);

// the length says 1000 bytes but only 3 follow
fp = fopen(TEST_FILE, "wb");
serialize_size(1000, fp);
fwrite("abc", 1, 3, fp);
fclose(fp);
fp = fopen(TEST_FILE, "rb");
CHECK(deserialize_string(fp) == NULL);
fclose(fp);

free(long_string);
remove(TEST_FILE);

END_TEST

START_TEST(serialize_test4)

int sizes[] = {0, 1, 10, 1000};
for (int s = 0; s < 4; s++) {
  array_ser_t *a = array_ser_init();
  for (int i = 0; i < sizes[s]; i++) {
    array_ser_append(a, i * 3 - 7);
  }
  CHECK(array_ser_serialize(a, TEST_FILE) == 0);

  array_ser_t *b = array_ser_deserialize(TEST_FILE);
  CHECK(b != NULL);
  CHECK(array_ser_size(b) == (size_t) sizes[s]);
  for (int i = 0; i < sizes[s]; i++) {
    CHECK(array_ser_get(b, i) == i * 3 - 7);
  }

  // the loaded array is a normal array
  CHECK(array_ser_append(b, 42) == 0);
  CHECK(array_ser_get(b, array_ser_size(b) - 1) == 42);

  array_ser_destroy(&a);
  array_ser_destroy(&b);
}

remove(TEST_FILE);

END_TEST

// A valid file cut off at every possible length must be rejected cleanly.
START_TEST(serialize_test5)

array_ser_t *a = array_ser_init2(20, 5);
CHECK(array_ser_serialize(a, TEST_FILE) == 0);
array_ser_destroy(&a);

char buf[4096];
long n = read_bytes(TEST_FILE, buf, sizeof(buf));
CHECK(n > 8);

for (long cut = 0; cut < n; cut++) {
  write_bytes(TEST_FILE, buf, cut);
  array_ser_t *b = array_ser_deserialize(TEST_FILE);
  CHECK(b == NULL);
}

write_bytes(TEST_FILE, buf, n);
array_ser_t *full = array_ser_deserialize(TEST_FILE);
CHECK(full != NULL);
CHECK(array_ser_size(full) == 20);
array_ser_destroy(&full);

remove(TEST_FILE);

END_TEST

START_TEST(serialize_test6)

array_serd_t *d = array_serd_init2(3, 1.5);
CHECK(array_serd_serialize(d, TEST_FILE) == 0);
array_serd_destroy(&d);

// wrong element type
CHECK(array_ser_deserialize(TEST_FILE) == NULL);

array_serd_t *ok = array_serd_deserialize(TEST_FILE);
CHECK(ok != NULL);
CHECK(array_serd_get(ok, 2) == 1.5);
array_serd_destroy(&ok);

write_bytes(TEST_FILE, "not an array file at all", 24);
CHECK(array_ser_deserialize(TEST_FILE) == NULL);

CHECK(array_ser_deserialize("serialize_test_missing.bin") == NULL);
CHECK(array_ser_deserialize(NULL) == NULL);

remove(TEST_FILE);

END_TEST

START_TEST(serialize_test7)

array_ser_t *a = array_ser_init2(10, 1);

CHECK(array_ser_serialize(NULL, TEST_FILE) == -1);
CHECK(array_ser_serialize(a, NULL) == -1);
CHECK(array_ser_serialize(a, "/nonexistent_dir/x.bin") == -1);
CHECK(array_ser_serialize(a, "/dev/full") == -1);

array_ser_destroy(&a);

END_TEST

// An element count that would overflow the byte size must not be trusted.
START_TEST(serialize_test8)

FILE *fp = fopen(TEST_FILE, "wb");
fwrite("Array===", 1, 8, fp);
serialize_string("ser", fp);
size_t data_size = sizeof(int);
size_t count = SIZE_MAX;
fwrite(&data_size, sizeof(size_t), 1, fp);
fwrite(&count, sizeof(size_t), 1, fp);
fclose(fp);
CHECK(array_ser_deserialize(TEST_FILE) == NULL);

count = SIZE_MAX / sizeof(int) + 1;
fp = fopen(TEST_FILE, "wb");
fwrite("Array===", 1, 8, fp);
serialize_string("ser", fp);
fwrite(&data_size, sizeof(size_t), 1, fp);
fwrite(&count, sizeof(size_t), 1, fp);
fclose(fp);
CHECK(array_ser_deserialize(TEST_FILE) == NULL);

remove(TEST_FILE);

END_TEST
