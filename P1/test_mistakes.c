#include <stdio.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    char *p;
    int local = 0;

    if (argc != 2 || argv[1][0] < '1' || argv[1][0] > '5'
                  || argv[1][1] != '\0') {
        fprintf(stderr, "Usage: %s 1|2|3|4|5\n", argv[0]);
        return 1;
    }

    switch (argv[1][0]) {
        case '1': /* Pointer outside the heap. */
            free(&local);
            return 1;

        case '2': /* Pointer inside an object. */
            p = malloc(16);
            if (p == NULL) return 1;
            free(p + 1);
            return 1;

        case '3': /* Double free. */
            p = malloc(16);
            if (p == NULL) return 1;
            free(p);
            free(p);
            return 1;

        case '4': /* Request too large for heap plus metadata. */
            p = malloc(4096);
            if (p == NULL) return 0;
            free(p);
            return 1;

        case '5': /* Deliberately leak one object. */
            p = malloc(20);
            if (p == NULL) return 1;
            return 0;
    }

    return 1;
}
