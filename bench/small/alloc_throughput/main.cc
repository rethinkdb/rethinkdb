
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../../../src/utils.hpp"
#include "../../../src/malloc_alloc.hpp"
#include "../../../src/pool_alloc.hpp"

void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

#define REPEAT        1000
#define NOBJECTS      1000000
#define OBJECT_SIZE   4 * 12
#define USEC          1000000L

int main() {
    void *objects[NOBJECTS];
    timeval tvb, tve;
    gettimeofday(&tvb, NULL);

    //malloc_alloc_t pool;
    pool_alloc_t<malloc_alloc_t> pool(NOBJECTS, OBJECT_SIZE);
    for(int c = 0; c < REPEAT; c++) {
        for(int i = 0; i < NOBJECTS; i++) {
            objects[i] = pool.malloc(OBJECT_SIZE);
            pool.free(objects[i]);
        }
    }
        
    gettimeofday(&tve, NULL);

    long bu = tvb.tv_sec * USEC + tvb.tv_usec;
    long eu = tve.tv_sec * USEC + tve.tv_usec;
    float diff = (float)(eu - bu) / USEC;

    printf("time: %f.2s\n", diff);
    
}
