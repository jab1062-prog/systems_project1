#include <stdio.h>
#include "mymalloc.h"

#define MEMLENGTH 4096

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;

static int initialized = 0;

/* Add your block metadata, helpers, mymalloc, and myfree here. */