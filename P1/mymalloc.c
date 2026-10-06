#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

#ifndef MEMLENGTH
#define MEMLENGTH 4096
#endif
#define HEADER_SIZE 8u
#define ALIGNMENT 8u

_Static_assert(sizeof(uint64_t) == HEADER_SIZE, "Header must occupy eight bytes");
_Static_assert(MEMLENGTH >= 16 && MEMLENGTH % ALIGNMENT == 0,
               "Heap length must be a multiple of eight and at least sixteen");

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;
static int initialized;

/* The header stores total chunk length, with bit zero indicating allocation.
 * memcpy avoids interpreting the character array as a different object type.
 * No allocator bookkeeping persists outside the heap except initialized. */
static uint64_t read_header(size_t offset)
{
    uint64_t header;
    memcpy(&header, heap.bytes + offset, HEADER_SIZE);
    return header;
}

static size_t chunk_size(uint64_t header)
{
    return (size_t)(header & ~UINT64_C(7));
}

static void write_header(size_t offset, size_t length, int allocated)
{
    uint64_t header = (uint64_t)length | (allocated ? UINT64_C(1) : 0);
    memcpy(heap.bytes + offset, &header, HEADER_SIZE);
}

static void report_leaks(void)
{
    size_t bytes = 0, objects = 0;
    for (size_t offset = 0; offset < MEMLENGTH;) {
        uint64_t header = read_header(offset);
        size_t length = chunk_size(header);
        if (header & UINT64_C(1)) {
            bytes += length - HEADER_SIZE;
            ++objects;
        }
        offset += length;
    }
    if (objects != 0)
        fprintf(stderr, "mymalloc: %zu bytes leaked in %zu objects.\n", bytes, objects);
}

static void initialize(void)
{
    if (!initialized) {
        write_header(0, MEMLENGTH, 0);
        if (atexit(report_leaks) != 0) {
            fprintf(stderr, "mymalloc: Unable to register leak detector.\n");
            exit(1);
        }
        initialized = 1;
    }
}

static void *allocation_failure(size_t size, char *file, int line)
{
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

static void invalid_free(char *file, int line)
{
    fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
    exit(2);
}

/* Reclaim removed headers while merging; never examine an allocated payload. */
static void coalesce(void)
{
    for (size_t offset = 0; offset < MEMLENGTH;) {
        uint64_t header = read_header(offset);
        size_t length = chunk_size(header);
        if (!(header & UINT64_C(1))) {
            while (offset + length < MEMLENGTH) {
                uint64_t next = read_header(offset + length);
                if (next & UINT64_C(1))
                    break;
                length += chunk_size(next);
                write_header(offset, length, 0);
            }
        }
        offset += length;
    }
}

void *mymalloc(size_t size, char *file, int line)
{
    initialize();
    /* Check before adding alignment padding, including for SIZE_MAX requests. */
    if (size == 0 || size > (size_t)MEMLENGTH - HEADER_SIZE)
        return allocation_failure(size, file, line);

    size_t payload = (size + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
    size_t needed = HEADER_SIZE + payload;
    for (size_t offset = 0; offset < MEMLENGTH;) {
        uint64_t header = read_header(offset);
        size_t length = chunk_size(header);
        if (!(header & UINT64_C(1)) && length >= needed) {
            size_t remainder = length - needed;
            if (remainder >= HEADER_SIZE + ALIGNMENT) {
                write_header(offset + needed, remainder, 0);
                write_header(offset, needed, 1);
            } else {
                /* Keep the entire chunk if the remainder cannot form a chunk. */
                write_header(offset, length, 1);
            }
            return heap.bytes + offset + HEADER_SIZE;
        }
        offset += length;
    }
    return allocation_failure(size, file, line);
}

void myfree(void *pointer, char *file, int line)
{
    initialize();
    /* The assignment does not specify NULL; retain standard free(NULL) behavior. */
    if (pointer == NULL)
        return;

    /* Compare for equality only. Ordering or subtracting unrelated pointers
     * would not be portable C. Only exact allocated payload starts are valid. */
    for (size_t offset = 0; offset < MEMLENGTH;) {
        uint64_t header = read_header(offset);
        if (pointer == (void *)(heap.bytes + offset + HEADER_SIZE)) {
            if (!(header & UINT64_C(1)))
                invalid_free(file, line);
            write_header(offset, chunk_size(header), 0);
            coalesce();
            return;
        }
        offset += chunk_size(header);
    }
    invalid_free(file, line);
}
