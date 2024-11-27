#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// define "x" for system malloc, include for our versions. Don't do both.
#ifdef SYSTEM_MALLOC
#define xfree free
#define xmalloc malloc
#define xrealloc realloc
#else
#include "malloc.h"
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define LARGE_ALLOC_MIN 1024
#define LARGE_ALLOC_MAX (1024 * 1024)
#define REALLOC_PROBABILITY 0.1
#define LARGE_ALLOC_PROBABILITY 0.1

int rand_between(int min, int max) {
    return min + rand() % (max - min + 1);
}

#define TEST_SIZE 30

int main(int argc, char *argv[]) {
    srand(time(NULL));

    char *test_string = "Now is the time for all good people to come to the aid "
                        "of their country.";

    if (argc > 1) {
        test_string = argv[1];
    }

    char *ptrs[TEST_SIZE];

    for (int ix = 0; ix < TEST_SIZE; ix++) {
        int size;
        if ((double)rand() / RAND_MAX < LARGE_ALLOC_PROBABILITY) {
            size = rand_between(LARGE_ALLOC_MIN, LARGE_ALLOC_MAX);
        } else {
            size = rand_between(1, strlen(test_string) + 1);
        }
        fprintf(stderr, "\n\n\n[%d] size: %d\n", ix, size);

        ptrs[ix] = xmalloc(size);
        if (ptrs[ix] == NULL) {
            printf("[%d] malloc failed\n", ix);
            exit(1);
        }

        int len_to_copy = MIN(strlen(test_string), size - 1);

        strncpy(ptrs[ix], test_string, len_to_copy);
        ptrs[ix][len_to_copy] = '\0';

        fprintf(stderr, "[%x] '%s'\n", ix, ptrs[ix]);

        int index_to_free = rand_between(0, ix);
        if (ptrs[index_to_free]) {
            fprintf(stderr, "\n[%d] randomly freeing %p ('%s')\n", index_to_free,
                    ptrs[index_to_free], ptrs[index_to_free]);
            xfree(ptrs[index_to_free]);
            fprintf(stderr, "[%d] freed %p\n", index_to_free, ptrs[index_to_free]);
            ptrs[index_to_free] = NULL;
        }

        // Occasionally realloc buffers
        if (ptrs[ix] != NULL && (double)rand() / RAND_MAX < REALLOC_PROBABILITY) {
            size_t new_size;
            if (rand() % 2 == 0) {
                new_size = rand_between(size, LARGE_ALLOC_MAX);
            } else {
                new_size = rand_between(1, size);
            }
            ptrs[ix] = realloc(ptrs[ix], new_size);
            if (ptrs[ix] == NULL) {
                fprintf(stderr, "Reallocation failed\n");
                exit(1);
            }
        }
    }

    for (int ix = 0; ix < TEST_SIZE; ix++) {
        if (ptrs[ix]) {
            fprintf(stderr, "[%d] freeing %p (%s)\n", ix, ptrs[ix], ptrs[ix]);
            xfree(ptrs[ix]);
            fprintf(stderr, "[%d] freed %p\n", ix, ptrs[ix]);
        } else {
            fprintf(stderr, "[%d] already freed\n", ix);
        }
    }

    return 0;
}