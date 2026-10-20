CC = gcc
CFLAGS = -std=c99 -g -Wall

.PHONY: all clean
all: words

words: words.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ words.c

clean:
	rm -f words
