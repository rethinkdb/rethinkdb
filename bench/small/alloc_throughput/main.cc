
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <new>

#include "../../../src/utils.hpp"
#include "../../../src/sizeheap_alloc.hpp"
#include "../../../src/malloc_alloc.hpp"
#include "../../../src/memalign_alloc.hpp"
#include "../../../src/pool_alloc.hpp"
#include "../../../src/dynamic_pool_alloc.hpp"
#include "../../../src/objectheap_alloc.hpp"

// We want redefined operator new here as well as some functions
#include "../../../src/utils.cc"

#define REPEAT        10000000L
#define NOBJECTS      100
#define USEC          1000000L
#define THREADS       8

struct object_t {
    int foo, bar, baz;
};

void* run_test(void *arg) {
    object_t *objects[NOBJECTS];
    //sizeheap_alloc_t<pool_alloc_t<memalign_alloc_t<> > > pool;
    //malloc_alloc_t pool;
    //pool_alloc_t<malloc_alloc_t> pool(NOBJECTS, sizeof(object_t));
    //pool_alloc_t<memalign_alloc_t<> > pool(NOBJECTS, sizeof(object_t));
    objectheap_adapter_t<objectheap_alloc_t<dynamic_pool_alloc_t<pool_alloc_t<memalign_alloc_t<> > >, object_t>, object_t> pool;
    for(int c = 0; c < REPEAT; c++) {
        for(int i = 0; i < NOBJECTS; i++) {
            objects[i] = (object_t*)pool.malloc(sizeof(object_t));
            check("Could not allocate object (out of memory)", objects[i] == NULL);
            objects[i]->foo = 10;
        }
        for(int i = 0; i < NOBJECTS; i++) {
            pool.free(objects[i]);
        }
    }
}

int main() {
    timeval tvb, tve;
    pthread_t threads[THREADS];

    gettimeofday(&tvb, NULL);
    for(int i = 0; i < THREADS; i++) {
        pthread_create(&threads[i], NULL, run_test, NULL);
    }
    for(int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&tve, NULL);

    long bu = tvb.tv_sec * USEC + tvb.tv_usec;
    long eu = tve.tv_sec * USEC + tve.tv_usec;
    float diff = (float)(eu - bu) / USEC;

    printf("time: %.2fs\n", diff);
    
}
