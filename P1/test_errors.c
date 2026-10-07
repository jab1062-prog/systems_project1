#include <stdio.h>
#include "mymalloc.h"

static int same_string(char *a, char *b)
{
    int i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return a[i] == b[i];
}

static void check(int condition, char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s outside|interior|double|merged-double|oom|oversize|overflow|zero|null|leak|no-leak\n", argv[0]);
        return 1;
    }
    if (same_string(argv[1], "outside")) {
        int local = 0;
        free(&local);
        check(0, "outside pointer was accepted");

        
    } else if (same_string(argv[1], "interior")) {
        char *p = malloc(16);
        check(p != NULL, "setup allocation failed");
        free(p + 1);
        check(0, "interior pointer was accepted");

        
    } else if (same_string(argv[1], "double")) {
        void *p = malloc(16);
        check(p != NULL, "setup allocation failed");
        free(p);
        free(p);
        check(0, "double free was accepted");

        
    } else if (same_string(argv[1], "merged-double")) {
        void *a = malloc(16);
        void *b = malloc(16);
        check(a != NULL && b != NULL, "setup allocation failed");
        free(a);
        free(b);
        free(b);
        check(0, "double free after merging was accepted");

        
    } else if (same_string(argv[1], "oom")) {
        void *p = malloc(4088);
        check(p != NULL, "full-heap setup allocation failed");
        check(malloc(1) == NULL, "allocation succeeded in a full heap");
        free(p);
        p = malloc(4088);
        check(p != NULL, "failed allocation damaged the heap");
        free(p);

        
    } else if (same_string(argv[1], "oversize")) {
        check(malloc(4089) == NULL, "oversized allocation succeeded");

        
    } else if (same_string(argv[1], "overflow")) {
        check(malloc((size_t)-1) == NULL, "huge request was not rejected");

        
    } else if (same_string(argv[1], "zero")) {
        check(malloc(0) == NULL, "zero-byte allocation policy changed");

        
    } else if (same_string(argv[1], "null")) {
        free(NULL);

        
    } else if (same_string(argv[1], "leak")) {
        void *a = malloc(1);
        void *b = malloc(20);
        void *c = malloc(8);
        check(a != NULL && b != NULL && c != NULL, "setup allocation failed");
        free(c);

        
    } else if (same_string(argv[1], "no-leak")) {
        void *p = malloc(20);
        check(p != NULL, "setup allocation failed");
        free(p);

        
    } else {
        fprintf(stderr, "Unknown test case: %s\n", argv[1]);
        return 1;
    }
    return 0;
}
