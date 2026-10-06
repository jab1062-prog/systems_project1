#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

static void alignment_and_data(void)
{
    unsigned char *p[24];
    for (size_t i = 0; i < 24; ++i) {
        p[i] = malloc(i + 1);
        assert(p[i] != NULL && (uintptr_t)p[i] % 8 == 0);
        memset(p[i], (int)(i + 1), i + 1);
    }
    for (size_t i = 0; i < 24; ++i)
        for (size_t j = 0; j <= i; ++j)
            assert(p[i][j] == i + 1);
    /* Also exercise properly aligned eight-byte objects. */
    double *d = malloc(sizeof *d);
    assert(d != NULL);
    *d = 3.25;
    for (int i = 23; i >= 0; --i) {
        for (int j = 0; j <= i; ++j)
            assert(p[i][j] == (unsigned char)(i + 1));
        free(p[i]);
    }
    assert(*d == 3.25);
    free(d);
}

static void exact_fit_and_small_remainder(void)
{
    /* Default heap has one eight-byte header and 4088 payload bytes. */
    unsigned char *p = malloc(4088);
    assert(p != NULL);
    memset(p, 0xA5, 4088);
    for (int i = 0; i < 4088; ++i)
        assert(p[i] == 0xA5);
    free(p);
    /* An eight-byte leftover cannot hold header + payload; keep it attached. */
    p = malloc(4080);
    assert(p != NULL);
    memset(p, 0x5A, 4080);
    free(p);
    p = malloc(4088);
    assert(p != NULL);
    free(p);
}

static void reuse_and_coalescing(void)
{
    void *a = malloc(64), *b = malloc(64), *c = malloc(64);
    unsigned char *guard = malloc(3872); /* Exhaust the default heap exactly. */
    assert(a && b && c && guard);
    memset(guard, 0xCC, 3872);
    free(b);
    void *again = malloc(64); /* Only the exact-size hole can satisfy this. */
    assert(again == b);
    free(again);
    free(a);
    void *merged = malloc(136); /* 64 + reclaimed header + 64. */
    assert(merged == a);
    free(merged);
    free(c);
    for (int i = 0; i < 3872; ++i)
        assert(guard[i] == 0xCC);
    free(guard);
    void *whole = malloc(4088);
    assert(whole != NULL);
    free(whole);
    /* Merge simultaneously with free neighbors on both sides. */
    a = malloc(64); b = malloc(64); c = malloc(64);
    guard = malloc(3872);
    assert(a && b && c && guard);
    free(a); free(c); free(b);
    merged = malloc(208);
    assert(merged == a);
    free(merged); free(guard);
}

static void random_data_preservation(void)
{
    unsigned char *p[32] = {0};
    size_t sizes[32] = {0};
    srand(214);
    for (int step = 0; step < 3000; ++step) {
        for (int i = 0; i < 32; ++i)
            if (p[i])
                for (size_t j = 0; j < sizes[i]; ++j)
                    assert(p[i][j] == (unsigned char)(i + 1));
        int slot = rand() % 32;
        if (p[slot]) {
            free(p[slot]);
            p[slot] = NULL;
        } else {
            sizes[slot] = 1u + (size_t)(rand() % 64);
            p[slot] = malloc(sizes[slot]);
            assert(p[slot] && (uintptr_t)p[slot] % 8 == 0);
            memset(p[slot], slot + 1, sizes[slot]);
        }
    }
    for (int i = 0; i < 32; ++i)
        if (p[i])
            free(p[i]);
    void *whole = malloc(4088);
    assert(whole != NULL);
    free(whole);
}

int main(void)
{
    free(NULL); /* Exercise initialization through free first. */
    alignment_and_data();
    exact_fit_and_small_remainder();
    reuse_and_coalescing();
    random_data_preservation();
    puts("All correctness tests passed.");
    return 0;
}
