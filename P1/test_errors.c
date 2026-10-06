#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s outside|interior|double|merged-double|oom|oversize|overflow|zero|null|leak|no-leak\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "outside") == 0) {
        int local;
        free(&local);
    } else if (strcmp(argv[1], "interior") == 0) {
        char *p = malloc(16);
        assert(p);
        free(p + 1);
    } else if (strcmp(argv[1], "double") == 0) {
        void *p = malloc(16);
        assert(p);
        free(p);
        free(p);
    } else if (strcmp(argv[1], "merged-double") == 0) {
        void *a = malloc(16), *b = malloc(16);
        assert(a && b);
        free(a); free(b); free(b);
    } else if (strcmp(argv[1], "oom") == 0) {
        void *p = malloc(4088);
        assert(p);
        assert(malloc(1) == NULL);
        free(p);
        p = malloc(4088);
        assert(p);
        free(p);
    } else if (strcmp(argv[1], "oversize") == 0) {
        assert(malloc(4089) == NULL);
    } else if (strcmp(argv[1], "overflow") == 0) {
        assert(malloc(SIZE_MAX) == NULL);
    } else if (strcmp(argv[1], "zero") == 0) {
        assert(malloc(0) == NULL);
    } else if (strcmp(argv[1], "null") == 0) {
        free(NULL);
    } else if (strcmp(argv[1], "leak") == 0) {
        void *a = malloc(1), *b = malloc(20), *c = malloc(8);
        assert(a && b && c);
        free(c); /* Two remaining objects: 8 + 24 aligned payload bytes. */
    } else if (strcmp(argv[1], "no-leak") == 0) {
        void *p = malloc(20);
        assert(p);
        free(p);
    } else {
        fprintf(stderr, "Unknown test case: %s\n", argv[1]);
        return 1;
    }
    return 0;
}
