
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "heap_alloc.hpp"

#define ITERATIONS  1
#define NOBJECTS    100
#define THREADS     2

void run_test(heap_t *heap, bool use_heap) {
    int* ptrs[NOBJECTS];
    for(int i = 0; i < ITERATIONS; i++) {
        for(int j = 0; j < NOBJECTS; j++) {
            if(use_heap) {
                ptrs[j] = (int*)heap_alloc(heap);
            } else {
                ptrs[j] = (int*)malloc(sizeof(int));
            }
            *ptrs[j] = j;
        }
        for(int j = 0; j < NOBJECTS / 2; j += 2) {
            if(use_heap) {
                heap_free(heap, ptrs[j]);
            } else {
                free(ptrs[j]);
            }
            ptrs[j] = NULL;
        }
        for(int j = 0; j < NOBJECTS / 2; j++) {
            if(use_heap) {
                ptrs[j] = (int*)heap_alloc(heap);
            } else {
                ptrs[j] = (int*)malloc(sizeof(int));
            }
            *ptrs[j] = NOBJECTS - j;
        }
        for(int j = 0; j < NOBJECTS; j++) {
            printf("%d, ", *ptrs[j]);
        }
    }
}

void* run_test(void *arg) {
    int use_heap = (int)(long)arg;
    if(use_heap) {
        heap_t heap;
        create_heap(&heap, sizeof(int), NOBJECTS);
        run_test(&heap, true);
        destroy_heap(&heap);
    } else {
        run_test(NULL, false);
    }
}

int main(int argc, char *argv[]) {
    int use_heap;

    if(argc == 2 && strcmp(argv[1], "heap") == 0) {
        use_heap = 1;
    }
    else if(argc == 2 && strcmp(argv[1], "malloc") == 0) {
        use_heap = 0;
    }
    else {
        printf("heap/malloc?\n");
        exit(-1);
    }

    pthread_t threads[THREADS];
    for(int i = 0; i < THREADS; i++) {
        pthread_create(&threads[i], NULL, run_test, (void*)use_heap);
    }
    for(int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}
