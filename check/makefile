CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall
INCLUDE = -I../include -I./


array_tests: array_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) array_tests.c -o array_tests

clean:
	rm -rf array_tests
