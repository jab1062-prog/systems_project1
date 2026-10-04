CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g

.PHONY: all clean

all: memgrind test_malloc

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) memgrind.c mymalloc.c -o memgrind

test_malloc: test_malloc.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) test_malloc.c mymalloc.c -o test_malloc

clean:
	rm -f memgrind test_malloc *.o