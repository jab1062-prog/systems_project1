CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -g

mysh: mysh.c
	$(CC) $(CFLAGS) -o $@ $<

sanitize:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o mysh mysh.c

clean:
	rm -f mysh

.PHONY: sanitize clean
