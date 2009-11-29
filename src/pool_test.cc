
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "utils.hpp"
#include "malloc_alloc.hpp"
#include "memalign_alloc.hpp"
#include "pool_alloc.hpp"
#include "sizeheap_alloc.hpp"
#include "objectheap_alloc.hpp"
#include "dynamic_pool_alloc.hpp"
#include "alloc_stats.hpp"

struct object_t {
    int foo;
    int bar;
    int baz;
};

#define NOBJECTS    155

int run_test(int i) {
    //sizeheap_alloc_t<pool_alloc_t<memalign_alloc_t<> > > pool;
    pool_alloc_t<malloc_alloc_t> pool(NOBJECTS, sizeof(object_t));
    
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

int run_test_dynamic(int i) {
    //objectheap_adapter_t<objectheap_alloc_t<dynamic_pool_alloc_t<pool_alloc_t<memalign_alloc_t<> > >, object_t>, object_t> pool;
    dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > > pool(sizeof(object_t));
    
    void *ptr;
    object_t *obj;
    object_t *objects[NOBJECTS];

    int count;
    for(count = 0; count < NOBJECTS; count++) {
        ptr = pool.malloc(sizeof(object_t));
        if(ptr == NULL) {
            break;
        }
        obj = (object_t*)ptr;
        objects[count] = obj;
        obj->foo = i * 3 + 1;
        obj->bar = i * 3 + 2;
        obj->baz = i * 3 + 3;
        i++;
    }
    printf("count: %d\n", count);
    check("Could not allocate all objects", count != NOBJECTS);

    for(int j = 0; j < NOBJECTS; j++) {
        obj = objects[j];
        printf("%d: (%d, %d, %d)\n", j + 1, obj->foo, obj->bar, obj->baz);
        pool.free((void*)obj);
    }

    pool.gc();

    return i;
}

int main(int argc, char *argv[]) {

    // Rudimentary test for multiple object allocation
    objectheap_alloc_t<dynamic_pool_alloc_t<pool_alloc_t<malloc_alloc_t> >, long, unsigned long> pool;
    pool.malloc<long>();
    pool.malloc<unsigned long>();

    // TODO: automate test (make sure numbers aren't overwritten)
    // TODO: create a test suite
    /*
    int i = run_test(0);
    printf("\n");
    run_test(i);
    */
    int i = run_test_dynamic(0);
    printf("\n");
    run_test_dynamic(i);
}
