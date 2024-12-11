#include <stdlib.h>

#ifndef MALLOC_H
#define MALLOC_H

typedef unsigned long long heap_key_t;



 typedef struct chunk_on_heap{
    int size;
    int free;  // 1 if free 0 if not
    char * pointer_to_start;
     unsigned long long as_int;
}chunk_on_heap_t;

typedef struct {
    heap_key_t key;
    chunk_on_heap_t chunk;
} heap_node_t;

typedef struct {
    heap_node_t *data;
    int size;
    int capacity;
} heap_t;


void * xmalloc(size_t size);
void xfree(void * ptr);
void *xrealloc(void *ptr, size_t size);

#endif