
#include "retest.hpp"
#include "alloc/malloc.hpp"
#include "alloc/pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/dynamic_pool.hpp"

struct object_t {
    int foo, bar;
};

void test_dynamic_alloc_free() {
    dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<malloc_alloc_t> > > pool(sizeof(object_t));

    // Initially dynamic pool allocates space for
    // DYNAMIC_POOL_INITIAL_NOBJECTS. Let's do one more object so that
    // dynamic pool allocates one more pool.
    object_t *objects[DYNAMIC_POOL_INITIAL_NOBJECTS + 1];
    for(int i = 0; i < DYNAMIC_POOL_INITIAL_NOBJECTS + 1; i++) {
        objects[i] = (object_t*)pool.malloc(sizeof(object_t));
        objects[i]->foo = i;
    }
    // At this point we shouldn't be able to garbage collect anything
    assert_eq(pool.gc(), 0);

    // We free the last object in the first pool. There should still
    // be an object in the second bucket, and the first bucket has
    // lots of objects, so we shouldn't be able to gc anything.
    assert_eq(objects[DYNAMIC_POOL_INITIAL_NOBJECTS - 1]->foo,
              DYNAMIC_POOL_INITIAL_NOBJECTS - 1);
    pool.free(objects[DYNAMIC_POOL_INITIAL_NOBJECTS - 1]);
    assert_eq(pool.gc(), 0);
    
    // Now we free the object in the second bucket. At this point gc
    // should collect the second bucket since it's empty.
    assert_eq(objects[DYNAMIC_POOL_INITIAL_NOBJECTS]->foo,
              DYNAMIC_POOL_INITIAL_NOBJECTS);
    pool.free(objects[DYNAMIC_POOL_INITIAL_NOBJECTS]);
    assert_eq(pool.gc(), 1);

    // Free all the objects (and make sure the values are right). We
    // shouldn't be able to gc anything because dynamic pool should
    // have at least one bucket.
    for(int i = 0; i < DYNAMIC_POOL_INITIAL_NOBJECTS - 1; i++) {
        assert_eq(objects[i]->foo, i);
        pool.free(objects[i]);
    }
    assert_eq(pool.gc(), 0);
}

