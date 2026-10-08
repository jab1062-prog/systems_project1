#include <stdio.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    char *p;
    int local = 0;

    if (argc != 2) {
        printf("Run with a test number from 1 to 5.\n");
        return 1;
    }

    switch (argv[1][0]) 
    {
        case '1': 
            free(&local);
            return 1;

        case '2': 
            p = malloc(16);
            if (p == NULL) return 1;
            free(p + 1);
            return 1;

        case '3': 
            p = malloc(16);
            if (p == NULL) return 1;
            free(p);
            free(p);
            return 1;

        case '4': 
            p = malloc(4096);
            if (p == NULL) return 0;
            free(p);
            return 1;

        case '5': 
            p = malloc(20);
            if (p == NULL) return 1;
            return 0;
    }

    return 1;
}
