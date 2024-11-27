#include "malloc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "malloc.h"

#define KEY_NOT_PRESENT -1
#define MAX_HEAP_CAPACITY (128 * 1024 * 1024)
#define META_SIZE sizeof(struct chunk_on_heap)

heap_t *free_chunks_heap = NULL;

void init_free_chunks_heap() {
    if (free_chunks_heap == NULL) {
        free_chunks_heap = sbrk(sizeof(heap_t));
        if (free_chunks_heap == (void *)-1) {
            fprintf(stderr, "Failed to initialize free_chunks_heap\n");
            exit(1);
        }
        free_chunks_heap->size = 0;
        free_chunks_heap->capacity = MAX_HEAP_CAPACITY;
        free_chunks_heap->data = sbrk(MAX_HEAP_CAPACITY * sizeof(heap_node_t));
        if (free_chunks_heap->data == (void *)-1) {
            fprintf(stderr, "Failed to initialize free_chunks_heap data\n");
            exit(1);
        }
    }
}

unsigned int heap_left_child(unsigned int index) {
    if (index == 0) return 1;
    return (index * 2) + 1;
}

unsigned int heap_right_child(unsigned int index) {
    if (index == 0) return 2;

    return (index * 2) + 2;
}
int heap_size(heap_t *heap) { return heap->size; }

unsigned int heap_parent(unsigned int index) {
    if (index <= 2) return 0;

    return (index - 1) / 2;
}
void heap_swap(heap_t *heap, int index1, int index2) {
    heap_node_t temp = heap->data[index1];

    heap->data[index1] = heap->data[index2];
    heap->data[index2] = temp;
}

void heap_bubble_up(heap_t *heap, int index) {
    int parent_index = heap_parent(index);

    while (index > 0 &&
           heap->data[index].chunk.size < heap->data[parent_index].chunk.size) {
        heap_swap(heap, index, parent_index);

        index = parent_index;

        parent_index = heap_parent(index);
    }
}

void heap_insert(heap_t *heap, heap_key_t key, chunk_on_heap_t data) {
    if (heap_size(heap) == heap->capacity) {
        return;
    }

    heap->data[heap_size(heap)].key = key;
    heap->data[heap_size(heap)].chunk = data;
    heap->size++;

    heap_bubble_up(heap, heap_size(heap) - 1);
}

void print_chunks() {
    for (int i = 0; i < free_chunks_heap->size; i++) {
        printf("Size: %d, Free: %d, Chunk Address: %p\n",
               free_chunks_heap->data[i].chunk.size,
               free_chunks_heap->data[i].chunk.free,
               (void *)free_chunks_heap->data[i].chunk.pointer_to_start);
    }
}

chunk_on_heap_t request_memory_from_system(size_t size) {

    void *block = sbrk(size + META_SIZE + (size * 4));
    if (block == (void *)-1) {
        return (chunk_on_heap_t){.size = 0, .pointer_to_start = NULL};
    }

    // First chunk
    chunk_on_heap_t chunk = {
        .size = size,
        .pointer_to_start =
            (char *)block + META_SIZE,  
        .free = 0};

    // Leftover chunk
    chunk_on_heap_t leftover = {
        .size = (size * 4) -
                META_SIZE,  
        .pointer_to_start = (char *)chunk.pointer_to_start + size + META_SIZE,
        .free = 1};

    // Insert chunks into the heap
    heap_insert(free_chunks_heap, chunk.size, chunk);
    heap_insert(free_chunks_heap, leftover.size, leftover);


    return chunk;
}

chunk_on_heap_t *heap_find_free_block(size_t size) {
    if (free_chunks_heap->size == 0) return NULL;

    for (int i = 0; i < free_chunks_heap->size; i++) {
        if (free_chunks_heap->data[i].chunk.size >= size &&
            free_chunks_heap->data[i].chunk.free) {
            free_chunks_heap->data[i].chunk.free = 0;

         
            if (free_chunks_heap->data[i].chunk.size > size + META_SIZE) {
                chunk_on_heap_t *leftover =
                    (chunk_on_heap_t *)((char *)free_chunks_heap->data[i]
                                            .chunk.pointer_to_start +
                                        size + META_SIZE);
                leftover->size =
                    free_chunks_heap->data[i].chunk.size - size - META_SIZE;
                leftover->free = 1;
                leftover->pointer_to_start = (char *)leftover + META_SIZE;

           
                free_chunks_heap->data[i].chunk.size = size;

                
                chunk_on_heap_t *leftover_meta =
                    (chunk_on_heap_t *)((char *)leftover->pointer_to_start -
                                        META_SIZE);
                *leftover_meta = *leftover;

              
                heap_insert(free_chunks_heap, leftover->size, *leftover_meta);

            }

            return &free_chunks_heap->data[i].chunk;
        }
    }

    return NULL;  // No suitable block found
}

void *xmalloc(size_t size) {
    if (size <= 0) return NULL;

    if (free_chunks_heap == NULL) init_free_chunks_heap();

    chunk_on_heap_t *block = heap_find_free_block(size);

    if (block == NULL) {
        chunk_on_heap_t chunk = request_memory_from_system(size);
        if (chunk.size == 0) return NULL;

        block = &free_chunks_heap->data[free_chunks_heap->size - 1].chunk;
    }

    chunk_on_heap_t *meta =
        (chunk_on_heap_t *)((char *)block->pointer_to_start - META_SIZE);
    *meta = *block;

    memset(block->pointer_to_start, 0, block->size);
  //  print_chunks();
    return block->pointer_to_start;
}

// Free memory
void xfree(void *ptr) {
    if (!ptr) return;

    chunk_on_heap_t *chunk = (chunk_on_heap_t *)((char *)ptr - META_SIZE);

    for (int i = 0; i < free_chunks_heap->size; i++) {
        if (free_chunks_heap->data[i].chunk.pointer_to_start ==
            chunk->pointer_to_start) {
            free_chunks_heap->data[i].chunk.free = 1;
            break;
        }
    }
}

void *xrealloc(void *ptr, size_t size) {
    if (!ptr) return xmalloc(size);

    if (size == 0) {
        xfree(ptr);
        return NULL;
    }

    chunk_on_heap_t *chunk = (chunk_on_heap_t *)((char *)ptr - META_SIZE);

    if (size <= chunk->size) {
        return ptr;
    }

    void *new_ptr = xmalloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, chunk->size);

    xfree(ptr);

    return new_ptr;
}
