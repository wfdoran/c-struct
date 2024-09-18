CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall
INCLUDE = -I../include -I./


unit_tests: array_tests.o
	./make_header.awk array_tests.c > unit_tests.h
	./make_test.awk array_tests.c > unit_tests.c
	$(CC) $(CFLAGS) $(INCLUDE) unit_tests.c array_tests.o -o unit_tests

array_tests.o: array_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c array_tests.c -o array_tests.o

clean:
	rm -rf unit_tests.c
	rm -rf unit_tests.h
	rm -rf unit_tests
	rm -rf *.o


