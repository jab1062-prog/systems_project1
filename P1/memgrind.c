#include <stdio.h>
#include <sys/time.h>
#include "mymalloc.h"

static void *allocate(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "memgrind: unexpected allocation failure\n");
        exit(1);
    }
    return p;
}

static void task1(void)
{
    int sizes[] = {8, 16, 32, 64, 128, 512, 1024};
    void *p[7];
    for (int i = 0; i < 7; i++) {
        p[i] = allocate(sizes[i]);
    }
    for (int i = 6; i >= 0; i--) {
        free(p[i]);
    }
}


static void task2(void)
{
    void *p[120];
    for (int i = 0; i < 120; i++) {
        p[i] = allocate(1);
    }
    for (int i = 0; i < 120; i++) {
        free(p[i]);
    }
}


static void task3(void)
{
    void *p[120];
    int live = 0;
    int allocations = 0;

    while (allocations < 120) {
        if (rand() % 2 == 0) {
            p[live] = allocate(1);
            live++;
            allocations++;
        } else if (live != 0) {
            int chosen = rand() % live;
            free(p[chosen]);
            live--;
            p[chosen] = p[live];
        }
    }

    
    for (int i = 0; i < live; i++) {
        free(p[i]);
    }
}

static void task4(void)
{
    void *p[40];
    for (int i = 0; i < 40; i++) {
        p[i] = allocate(8 * (1 + i % 5));
    }
    for (int i = 0; i < 40; i += 2) {
        free(p[i]);
    }
    for (int i = 0; i < 40; i += 2) {
        p[i] = allocate(8);
    }
    for (int i = 1; i < 40; i += 2) {
        free(p[i]);
    }
    for (int i = 38; i >= 0; i -= 2) {
        free(p[i]);
    }
}



static void task5(void)
{
    void *p[30];
    void *replacement[10];
    for (int i = 0; i < 30; i++) {
        p[i] = allocate(64);
    }
    for (int group = 0; group < 10; group++) {
        free(p[group * 3 + 1]);
        free(p[group * 3]);
    }
    for (int group = 0; group < 10; group++) {
        replacement[group] = allocate(128);
    }
    for (int group = 9; group >= 0; group--) {
        free(p[group * 3 + 2]);
        free(replacement[group]);
    }
}




int main(void)
{
    struct timeval start, end;
    double elapsed;

    srand(214);
    if (gettimeofday(&start, NULL) != 0) {
        perror("gettimeofday");
        return 1;
    }
    for (int run = 0; run < 50; run++) {
        task1();
        task2();
        task3();
        task4();
        task5();
    }

    
    if (gettimeofday(&end, NULL) != 0) {
        perror("gettimeofday");
        return 1;
    }


    
    elapsed = (end.tv_sec - start.tv_sec) * 1000000.0
            + (end.tv_usec - start.tv_usec);
    printf("Average time for the five-task workload (50 runs): %.3f microseconds\n",
           elapsed / 50.0);
    return 0;
}
