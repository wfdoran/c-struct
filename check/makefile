CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall -fopenmp
INCLUDE = -I../include -I./

TESTS = assert array linked_list tree chan hash_table phash_table pqueue option serialize comp any interval link hook model pool ppool macro null
SRC = $(addsuffix _tests.c, $(TESTS))
OBJS = $(addsuffix _tests.o, $(TESTS))

# Every test object depends on all of the library headers and on check.h, so
# editing any of them rebuilds the tests.
HEADERS = $(wildcard ../include/*.h) check.h

unit_tests: $(OBJS) make_header.awk make_test.awk
	./make_header.awk $(SRC) > unit_tests.h
	./make_test.awk $(SRC) > unit_tests.c
	$(CC) $(CFLAGS) $(INCLUDE) unit_tests.c $(OBJS) -o unit_tests -lpthread -lm

%_tests.o: %_tests.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# the interval code depends on the rounding mode being honored
interval_tests.o: CFLAGS += -frounding-math

# compiles the code examples of doc/*.md and README.md
.PHONY: doc-check
doc-check:
	CC=$(CC) sh doc_examples.sh

# ---------------------------------------------------------------------------
# Testing.  Each target stops at the first failure.
#
#   make test        unit tests, documentation examples, header formatting
#   make test-asan   unit tests built with AddressSanitizer and UBSan
#   make test-tsan   unit tests built with ThreadSanitizer (see below)
#   make test-all    test and test-asan
#
# The sanitizer builds use objects of their own (*.asan.o, *.tsan.o), so they do
# not mix with the plain build.  The sanitizers need the address space layout
# not to be randomized on some systems (they crash at random otherwise), so
# they run under setarch -R; set SETARCH= (empty) where that is not available.
#
# test-tsan does not pass yet, and is not part of test-all.  The threaded tests
# start their threads with OpenMP, and gcc's libgomp is not instrumented, so
# ThreadSanitizer cannot see that the work done before a parallel region
# happens before the threads and that the threads finish before the code after
# it.  Every report (31 on the last run) has the main thread's access on one
# side, e.g. phash_pi_init() against the first phash_pi_put() of a worker.
# Either run it with clang and the Archer OpenMP runtime, or write the
# threaded tests with pthreads, before relying on it.
# ---------------------------------------------------------------------------

SETARCH = setarch -R
SANFLAGS = -O1 -g -fno-omit-frame-pointer -std=gnu11 -Wall -Wextra -fopenmp
ASAN_FLAGS = $(SANFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined
TSAN_FLAGS = $(SANFLAGS) -fsanitize=thread

# the generated driver, also needed by the sanitizer builds
unit_tests.h: $(SRC) make_header.awk
	./make_header.awk $(SRC) > unit_tests.h

unit_tests.c: $(SRC) make_test.awk
	./make_test.awk $(SRC) > unit_tests.c

%_tests.asan.o: %_tests.c $(HEADERS)
	$(CC) $(ASAN_FLAGS) $(INCLUDE) -c $< -o $@

%_tests.tsan.o: %_tests.c $(HEADERS)
	$(CC) $(TSAN_FLAGS) $(INCLUDE) -c $< -o $@

interval_tests.asan.o interval_tests.tsan.o: SANFLAGS += -frounding-math

ASAN_OBJS = $(addsuffix _tests.asan.o, $(TESTS))
TSAN_OBJS = $(addsuffix _tests.tsan.o, $(TESTS))

unit_tests_asan: unit_tests.h unit_tests.c $(ASAN_OBJS)
	$(CC) $(ASAN_FLAGS) $(INCLUDE) unit_tests.c $(ASAN_OBJS) -o $@ -lpthread -lm

unit_tests_tsan: unit_tests.h unit_tests.c $(TSAN_OBJS)
	$(CC) $(TSAN_FLAGS) $(INCLUDE) unit_tests.c $(TSAN_OBJS) -o $@ -lpthread -lm

.PHONY: test test-asan test-tsan test-all
test: unit_tests
	./unit_tests
	$(MAKE) doc-check
	$(MAKE) -C ../include check-format

test-asan: unit_tests_asan
	$(SETARCH) ./unit_tests_asan

test-tsan: unit_tests_tsan
	$(SETARCH) ./unit_tests_tsan

test-all: test test-asan

.PHONY: clean
clean:
	rm -rf unit_tests.c
	rm -rf unit_tests.h
	rm -rf unit_tests
	rm -rf *.o
	rm -f unit_tests_asan unit_tests_tsan
