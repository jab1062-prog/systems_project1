CC = gcc
CFLAGS = -std=c99 -g -Wall

.PHONY: all tests check clean

all: memgrind

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) memgrind.c mymalloc.c -o memgrind

test_correctness: test_correctness.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) test_correctness.c mymalloc.c -o test_correctness

test_errors: test_errors.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) test_errors.c mymalloc.c -o test_errors

memtest: memtest.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) memtest.c mymalloc.c -o memtest

tests: test_correctness test_errors memtest

check: all tests
	./test_correctness
	./memtest
	./memgrind

clean:
	rm -f *.o memgrind test_correctness test_errors memtest
