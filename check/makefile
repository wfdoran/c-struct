CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall -fopenmp
INCLUDE = -I../include -I./

TESTS = assert array linked_list tree chan hash_table phash_table pqueue option serialize comp any interval link hook
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

.PHONY: clean
clean:
	rm -rf unit_tests.c
	rm -rf unit_tests.h
	rm -rf unit_tests
	rm -rf *.o
