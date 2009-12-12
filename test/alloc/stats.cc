
#include "retest.hpp"
#include "alloc/malloc.hpp"
#include "alloc/stats.hpp"

#define NMALLOCS 100

void test_empty() {
    alloc_stats_t<malloc_alloc_t> pool;

    void *ptrs[NMALLOCS];
    for(int i = 0; i < NMALLOCS; i++) {
        ptrs[i] = pool.malloc(10);
    }
    assert_cond(!pool.empty());

    for(int i = 0; i < NMALLOCS; i++) {
        pool.free(ptrs[i]);
    }
    assert_cond(pool.empty());
}

