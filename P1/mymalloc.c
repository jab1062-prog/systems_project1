#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

#ifndef MEMLENGTH
#define MEMLENGTH 4096
#endif

struct header
{
    int size;
    int allocated;  
};

#define HEADER_SIZE ((sizeof(struct header) + 7) & ~(size_t)7)

static union
{
    char bytes[MEMLENGTH];
    double not_used;
} 

heap;

static int initialized = 0;




static struct header read_header(int offset)
{
    struct header h;
    
    memcpy(&h, heap.bytes + offset, sizeof h);
    
    return h;
}



static void write_header(int offset, int size, int allocated)
{
    struct header h;
    h.size = size;
    h.allocated = allocated;
    
    memcpy(heap.bytes + offset, &h, sizeof h);
}

static void report_leaks(void)
{
    int offset = 0;
    int objects = 0;
    int bytes = 0;


    
    while (offset < MEMLENGTH)
    {
        struct header h = read_header(offset);
        if (h.allocated)
        {
            objects++;
            bytes += h.size - (int)HEADER_SIZE;
        }
        offset += h.size;
    }

    
    if (objects != 0)
    {
        fprintf(stderr, "mymalloc: %d bytes leaked in %d objects.\n", bytes, objects);
    }
}



static void initialize(void)
{
    if (!initialized)
    {
        write_header(0, MEMLENGTH, 0);
        if (atexit(report_leaks) != 0)
        {
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

static void coalesce(void)
{
    int offset = 0;

    while (offset < MEMLENGTH)
    {
        struct header h = read_header(offset);
        int next_offset = offset + h.size;

        if (!h.allocated)
        {
            
            while (next_offset < MEMLENGTH)
            {
                struct header next = read_header(next_offset);
                if (next.allocated)
                {
                    break;
                }
                h.size += next.size;
                
                write_header(offset, h.size, 0);
                
                next_offset = offset + h.size;
            }
        }

        
        offset += h.size;
    }
}

void *mymalloc(size_t size, char *file, int line)
{
    int offset = 0;
    int needed;

    initialize();

    
    if (size == 0 || size > MEMLENGTH - HEADER_SIZE)
    {
        return allocation_failure(size, file, line);
    }

    needed = (int)((size + 7) & ~(size_t)7) + (int)HEADER_SIZE;

    while (offset < MEMLENGTH)
    {
        struct header h = read_header(offset);
        if (!h.allocated && h.size >= needed)
        {
            
            int remaining = h.size - needed;
            
            if (remaining >= (int)HEADER_SIZE + 8)
            {
                write_header(offset + needed, remaining, 0);
                write_header(offset, needed, 1);
            } 
            
            else
            {
                write_header(offset, h.size, 1);
            }

            
            return heap.bytes + offset + HEADER_SIZE;
        }

        
        offset += h.size;
    }

    
    return allocation_failure(size, file, line);
}



void myfree(void *pointer, char *file, int line)
{
    int offset = 0;

    initialize();

    
    if (pointer == NULL)
    {
        return;
    }

    while (offset < MEMLENGTH)
    {
        struct header h = read_header(offset);
        
        if (pointer == (void *)(heap.bytes + offset + HEADER_SIZE))
        {
            if (!h.allocated)
            {
                
                invalid_free(file, line);
            }
            
            write_header(offset, h.size, 0);
            
            coalesce();
            
            return;
        }
        
        offset += h.size;
    }
    
    invalid_free(file, line);
}
