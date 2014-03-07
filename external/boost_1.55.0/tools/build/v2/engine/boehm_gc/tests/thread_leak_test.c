#ifndef GC_THREADS
#  define GC_THREADS
#endif /* GC_THREADS */
#include "leak_detector.h"
#include <pthread.h>
#include <stdio.h>

void * test(void * arg) {
    int *p[10];
    int i;
    GC_find_leak = 1; /* for new collect versions not compiled  */
    /* with -DFIND_LEAK.                                        */
    for (i = 0; i < 10; ++i) {
        p[i] = malloc(sizeof(int)+i);
    }
    CHECK_LEAKS();
    for (i = 1; i < 10; ++i) {
        free(p[i]);
    }
    return 0;
}       

#define NTHREADS 5

main() {
    int i;
    pthread_t t[NTHREADS];
    int code;

    GC_INIT();
    for (i = 0; i < NTHREADS; ++i) {
	if ((code = pthread_create(t + i, 0, test, 0)) != 0) {
    	    printf("Thread creation failed %d\n", code);
        }
    }
    for (i = 0; i < NTHREADS; ++i) {
	if ((code = pthread_join(t[i], 0)) != 0) {
            printf("Thread join failed %lu\n", code);
        }
    }
    CHECK_LEAKS();
    CHECK_LEAKS();
    CHECK_LEAKS();
    return 0;
}
