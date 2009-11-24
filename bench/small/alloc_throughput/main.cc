
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../../../src/utils.hpp"
#include "../../../src/malloc_alloc.hpp"
#include "../../../src/memalign_alloc.hpp"
#include "../../../src/pool_alloc.hpp"

void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

#define REPEAT        500000000L
#define NOBJECTS      10
#define OBJECT_SIZE   4 * 12
#define USEC          1000000L
#define THREADS       7

void* run_test(void *arg) {
    void *objects[NOBJECTS];
    // malloc_alloc_t pool;
    //pool_alloc_t<malloc_alloc_t> pool(NOBJECTS, OBJECT_SIZE);
    pool_alloc_t<memalign_alloc_t<> > pool(NOBJECTS, OBJECT_SIZE);
    for(int c = 0; c < REPEAT; c++) {
        for(int i = 0; i < NOBJECTS; i++) {
            objects[i] = pool.malloc(OBJECT_SIZE);
            *(int*)objects[i] = 10;
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
