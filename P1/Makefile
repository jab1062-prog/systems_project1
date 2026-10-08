CC = gcc
CFLAGS = -std=c99 -g -Wall

.PHONY: all tests check clean

all: memgrind

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) memgrind.c mymalloc.c -o memgrind

test_accuracy: test_accuracy.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) test_accuracy.c mymalloc.c -o test_accuracy

test_mistakes: test_mistakes.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) test_mistakes.c mymalloc.c -o test_mistakes

memtest: memtest.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) memtest.c mymalloc.c -o memtest

tests: test_accuracy test_mistakes memtest

check: all tests
	./test_accuracy
	./memtest
	./memgrind

clean:
	rm -f *.o memgrind test_accuracy test_mistakes memtest
