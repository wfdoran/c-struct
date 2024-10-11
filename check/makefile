CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall -Wno-unused-function -fopenmp
INCLUDE = -I../include -I./

SRC = array_tests.c linked_list_tests.c tree_tests.c chan_tests.c
OBJS = array_tests.o linked_list_tests.o tree_tests.o chan_tests.o

unit_tests: $(OBJS)
	./make_header.awk $(SRC) > unit_tests.h
	./make_test.awk $(SRC) > unit_tests.c
	$(CC) $(CFLAGS) $(INCLUDE) unit_tests.c $(OBJS) -o unit_tests

array_tests.o: array_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c array_tests.c -o array_tests.o

linked_list_tests.o: linked_list_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c linked_list_tests.c -o linked_list_tests.o

tree_tests.o: tree_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c tree_tests.c -o tree_tests.o

chan_tests.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c chan_tests.c -o chan_tests.o
clean:
	rm -rf unit_tests.c
	rm -rf unit_tests.h
	rm -rf unit_tests
	rm -rf *.o


