CC = gcc
CFLAGS = -std=c99 -g -Wall -Wextra -Wpedantic

.PHONY: all clean test
all: words

words: words.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ words.c

test: words
	python3 tests/test_words.py ./words

clean:
	rm -f words
