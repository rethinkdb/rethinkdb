
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "utils.hpp"
#include "malloc_alloc.hpp"
#include "memalign_alloc.hpp"
#include "pool_alloc.hpp"
#include "sizeheap_alloc.hpp"

struct object_t {
    int foo;
    int bar;
    int baz;
};

#define NOBJECTS    10

int run_test(int i) {
    sizeheap_alloc_t<pool_alloc_t<memalign_alloc_t<> > > pool;
    //pool_alloc_t<malloc_alloc_t> pool(NOBJECTS, sizeof(object_t));
    void *ptr;
    object_t *obj;
    object_t *objects[NOBJECTS];
    
    int count = 0;
    while((ptr = pool.malloc(sizeof(object_t))) != NULL) {
        obj = (object_t*)ptr;
        objects[count] = obj;
        obj->foo = i * 3 + 1;
        obj->bar = i * 3 + 2;
        obj->baz = i * 3 + 3;
        i++;
        count++;
    }
    check("Could not allocate all objects", count != NOBJECTS);
    check("Shouldn't be able to go beyond the pool", pool.malloc(sizeof(object_t)) != NULL);

    for(int j = 0; j < NOBJECTS; j++) {
        obj = objects[j];
        printf("%d: (%d, %d, %d)\n", j + 1, obj->foo, obj->bar, obj->baz);
        pool.free((void*)obj);
    }

    return i;
}

int main(int argc, char *argv[]) {
    // TODO: automate test (make sure numbers aren't overwritten)
    // TODO: create a test suite
    int i = run_test(0);
    printf("\n");
    run_test(i);
}
