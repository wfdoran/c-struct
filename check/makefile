CC = gcc
CFLAGS = -O2 -std=gnu11 -g -Wall -Wno-unused-function -fopenmp
INCLUDE = -I../include -I./

TESTS = array linked_list tree chan hash_table phash_table pqueue option serialize comp any
SRC = $(addsuffix _tests.c, $(TESTS))
OBJS = $(addsuffix _tests.o, $(TESTS))

unit_tests: $(OBJS)
	./make_header.awk $(SRC) > unit_tests.h
	./make_test.awk $(SRC) > unit_tests.c
	$(CC) $(CFLAGS) $(INCLUDE) unit_tests.c $(OBJS) -o unit_tests -lpthread -lm

array_tests.o: array_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c array_tests.c -o array_tests.o

linked_list_tests.o: linked_list_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c linked_list_tests.c -o linked_list_tests.o

tree_tests.o: tree_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c tree_tests.c -o tree_tests.o

chan_tests.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c chan_tests.c -o chan_tests.o

hash_table_tests.o: hash_table_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c hash_table_tests.c -o hash_table_tests.o

phash_table_tests.o: phash_table_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c phash_table_tests.c -o phash_table_tests.o

pqueue_tests.o: pqueue_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c pqueue_tests.c -o pqueue_tests.o

option_tests.o: option_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c option_tests.c -o option_tests.o

serialize_tests.o: serialize_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c serialize_tests.c -o serialize_tests.o

comp_tests.o: comp_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c comp_tests.c -o comp_tests.o

any_tests.o: any_tests.c check.h
	$(CC) $(CFLAGS) $(INCLUDE) -c any_tests.c -o any_tests.o

clean:
	rm -rf unit_tests.c
	rm -rf unit_tests.h
	rm -rf unit_tests
	rm -rf *.o


