
#include "retest.hpp"
#include "alloc/pool.hpp"
#include "alloc/malloc.hpp"

struct object_t {
    int foo, bar, baz;
};

#define NOBJECTS 100

void test_alloc_free() {
    pool_alloc_t<malloc_alloc_t> alloc(NOBJECTS, sizeof(object_t));
    object_t *objects[NOBJECTS];
    for(int i = 0; i < NOBJECTS; i++) {
        objects[i] = (object_t*)alloc.malloc(sizeof(object_t));
        objects[i]->foo = i * 3 + 0;
        objects[i]->bar = i * 3 + 1;
        objects[i]->baz = i * 3 + 2;
    }
    for(int i = 0; i < NOBJECTS; i++) {
        assert_eq(objects[i]->foo, i * 3 + 0);
        assert_eq(objects[i]->bar, i * 3 + 1);
        assert_eq(objects[i]->baz, i * 3 + 2);
        alloc.free((void*)objects[i]);
    }
}

void test_pool_overalloc() {
    pool_alloc_t<malloc_alloc_t> alloc(NOBJECTS, sizeof(object_t));
    for(int i = 0; i < NOBJECTS; i++) {
        alloc.malloc(sizeof(object_t));
    }
    assert_eq(alloc.malloc(sizeof(object_t)), (void*)NULL);
}

void test_pool_in_range() {
    pool_alloc_t<malloc_alloc_t> alloc(NOBJECTS, sizeof(object_t));
    void *ptr = alloc.malloc(sizeof(object_t));
    assert_cond(alloc.in_range(ptr));
    assert_cond(alloc.in_range((char*)ptr + sizeof(object_t) * NOBJECTS) - 1);
    assert_cond(!alloc.in_range((char*)ptr - 1));
    assert_cond(!alloc.in_range((char*)ptr + sizeof(object_t) * NOBJECTS));
}
