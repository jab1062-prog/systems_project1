#include <stdio.h>
#include "mymalloc.h"

static void check(int condition)
{
    if (!condition) {
        fprintf(stderr, "Accuracy test failed.\n");
        exit(1);
    }
}

int main(void)
{
    /* Fill the heap, including three 8-byte headers. */
    char *a = malloc(512);
    char *b = malloc(512);
    char *guard = malloc(3048);

    check(a != NULL && b != NULL && guard != NULL);
    check((b - a) % 8 == 0 && (guard - a) % 8 == 0);

    for (int i = 0; i < 512; i++) {
        a[i] = 1;
        b[i] = 2;
    }
    for (int i = 0; i < 3048; i++) {
        guard[i] = 3;
    }
    for (int i = 0; i < 512; i++) {
        check(a[i] == 1 && b[i] == 2);
    }

    /* Check that freed space can be reused. */
    char *old_b = b;
    free(b);
    b = malloc(512);
    check(b == old_b);

    /* Check that adjacent free chunks merge. */
    free(a);
    free(b);

    char *merged = malloc(1032);
    check(merged != NULL);

    for (int i = 0; i < 1032; i++) {
        merged[i] = 4;
    }
    for (int i = 0; i < 3048; i++) {
        check(guard[i] == 3);
    }

    free(merged);
    free(guard);

    double *number = malloc(sizeof(double));
    check(number != NULL);
    *number = 3.25;
    check(*number == 3.25);
    free(number);

    puts("All Accuracy tests passed.");
    return 0;
}
