CC = gcc
CFLAGS = -std=c11 -O2 -g -Wall -Wextra -Wpedantic

.PHONY: all tests check clean
all: memgrind

mymalloc.o: mymalloc.c mymalloc.h

memgrind: memgrind.o mymalloc.o

memgrind.o: memgrind.c mymalloc.h

test_correctness: test_correctness.o mymalloc.o

test_correctness.o: test_correctness.c mymalloc.h

test_errors: test_errors.o mymalloc.o

test_errors.o: test_errors.c mymalloc.h

tests: test_correctness test_errors

check: all tests
	sh ./run_tests.sh

clean:
	rm -f *.o memgrind test_correctness test_errors
