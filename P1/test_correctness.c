#include <stdio.h>
#include "mymalloc.h"

static void check(int condition, char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void alignment_and_data(void)
{
    char *p[24];
    double *d;

    for (int i = 0; i < 24; i++) {
        p[i] = malloc(i + 1);
        check(p[i] != NULL, "small allocation failed");
        /* All these pointers belong to the same heap array. */
        check((p[i] - p[0]) % 8 == 0, "payload spacing is not aligned");
        for (int j = 0; j <= i; j++) {
            p[i][j] = i + 1;
        }
    }
    d = malloc(sizeof(double));
    check(d != NULL, "double allocation failed");
    *d = 3.25;

    for (int i = 23; i >= 0; i--) {
        for (int j = 0; j <= i; j++) {
            check(p[i][j] == i + 1, "allocated objects overlap or lost data");
        }
        free(p[i]);
    }
    check(*d == 3.25, "freeing other objects changed the double");
    free(d);
}

static void exact_fit_and_small_remainder(void)
{
    /* These boundary tests assume the default heap and an 8-byte header. */
    char *p = malloc(4088);
    check(p != NULL, "full-heap allocation failed");
    for (int i = 0; i < 4088; i++) {
        p[i] = 42;
    }
    for (int i = 0; i < 4088; i++) {
        check(p[i] == 42, "full-heap allocation lost data");
    }
    free(p);

    /* An 8-byte remainder is too small for a new header plus payload. */
    p = malloc(4080);
    check(p != NULL, "small-remainder allocation failed");
    for (int i = 0; i < 4080; i++) {
        p[i] = 17;
    }
    free(p);
    p = malloc(4088);
    check(p != NULL, "small remainder was lost instead of reclaimed");
    free(p);
}

static void reuse_and_coalescing(void)
{
    void *a = malloc(64);
    void *b = malloc(64);
    void *c = malloc(64);
    char *guard = malloc(3872);
    void *again;
    void *merged;
    void *whole;

    check(a != NULL && b != NULL && c != NULL && guard != NULL,
          "could not fill heap for coalescing test");
    for (int i = 0; i < 3872; i++) {
        guard[i] = 73;
    }
    free(b);
    again = malloc(64);
    check(again == b, "free did not make the only available hole reusable");
    free(again);
    free(a);
    merged = malloc(136); /* 64 data + reclaimed 8-byte header + 64 data. */
    check(merged == a, "two adjacent free blocks did not merge");
    free(merged);
    free(c);
    for (int i = 0; i < 3872; i++) {
        check(guard[i] == 73, "merging free chunks changed live data");
    }
    free(guard);
    whole = malloc(4088);
    check(whole != NULL, "freeing everything did not restore the whole heap");
    free(whole);

    a = malloc(64);
    b = malloc(64);
    c = malloc(64);
    guard = malloc(3872);
    check(a != NULL && b != NULL && c != NULL && guard != NULL,
          "could not refill heap");
    free(a);
    free(c);
    free(b);
    merged = malloc(208);
    check(merged == a, "free block did not merge with both neighbors");
    free(merged);
    free(guard);
}

static void random_data_preservation(void)
{
    char *p[32];
    int sizes[32];
    void *whole;

    for (int i = 0; i < 32; i++) {
        p[i] = NULL;
        sizes[i] = 0;
    }
    srand(214);
    for (int step = 0; step < 3000; step++) {
        int slot;
        for (int i = 0; i < 32; i++) {
            if (p[i] != NULL) {
                for (int j = 0; j < sizes[i]; j++) {
                    check(p[i][j] == i + 1, "random operations changed live data");
                }
            }
        }
        slot = rand() % 32;
        if (p[slot] != NULL) {
            free(p[slot]);
            p[slot] = NULL;
        } else {
            sizes[slot] = 1 + rand() % 64;
            p[slot] = malloc(sizes[slot]);
            check(p[slot] != NULL, "random allocation failed");
            for (int j = 0; j < sizes[slot]; j++) {
                p[slot][j] = slot + 1;
            }
        }
    }
    /* Check final contents too, including the object from the last step. */
    for (int i = 0; i < 32; i++) {
        if (p[i] != NULL) {
            for (int j = 0; j < sizes[i]; j++) {
                check(p[i][j] == i + 1, "final object contents changed");
            }
            free(p[i]);
        }
    }
    whole = malloc(4088);
    check(whole != NULL, "random operations did not restore the whole heap");
    free(whole);
}

int main(void)
{
    free(NULL); /* Also test automatic initialization through free. */
    alignment_and_data();
    exact_fit_and_small_remainder();
    reuse_and_coalescing();
    random_data_preservation();
    puts("All correctness tests passed.");
    return 0;
}
