#include <stdio.h>
#include <sys/time.h>
#include "mymalloc.h"

static void *allocate(int size)
{
    void *p = malloc(size);
    if (p == NULL) {
        exit(1);
    }
    return p;
}

int main(void)
{
    struct timeval start, end;
    void *p[120];
    int sizes[] = {8, 16, 32, 64, 128, 512, 1024};

    if (gettimeofday(&start, NULL) != 0) {
        return 1;
    }

    for (int run = 0; run < 50; run++) {
        /* 1: Required sizes, freed backwards. */
        for (int i = 0; i < 7; i++) {
            p[i] = allocate(sizes[i]);
        }
        for (int i = 6; i >= 0; i--) {
            free(p[i]);
        }

        /* 2: 120 small objects, freed in order. */
        for (int i = 0; i < 120; i++) {
            p[i] = allocate(1);
        }
        for (int i = 0; i < 120; i++) {
            free(p[i]);
        }

        /* 3: Random choices until 120 total allocations. */
        int live = 0;
        int allocations = 0;

        while (allocations < 120) {
            if (rand() % 2 == 0) {
                p[live] = allocate(1);
                live++;
                allocations++;
            } else if (live > 0) {
                int chosen = rand() % live;
                free(p[chosen]);
                live--;
                p[chosen] = p[live];
            }
        }
        for (int i = 0; i < live; i++) {
            free(p[i]);
        }

        /* 4: Replace alternating objects, then free backwards. */
        for (int i = 0; i < 20; i++) {
            p[i] = allocate(64);
        }
        for (int i = 0; i < 20; i += 2) {
            free(p[i]);
            p[i] = allocate(32);
        }
        for (int i = 19; i >= 0; i--) {
            free(p[i]);
        }

        /* 5: Replace adjacent pairs with larger objects. */
        for (int i = 0; i < 12; i++) {
            p[i] = allocate(64);
        }
        for (int i = 0; i < 12; i += 3) {
            free(p[i]);
            free(p[i + 1]);
            p[i] = allocate(128);
            p[i + 1] = NULL;
        }
        for (int i = 11; i >= 0; i--) {
            if (p[i] != NULL) {
                free(p[i]);
            }
        }
    }

    if (gettimeofday(&end, NULL) != 0) {
        return 1;
    }

    double elapsed = (end.tv_sec - start.tv_sec) * 1000000.0
                   + (end.tv_usec - start.tv_usec);

    printf("Average time: %.3f microseconds\n", elapsed / 50.0);
    return 0;
}
