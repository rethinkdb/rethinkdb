
#include "retest.hpp"
#include "alloc/memalign.hpp"

#define ALIGNMENT  64

void test_alignment() {
    memalign_alloc_t<ALIGNMENT> alloc;
    char *ptr = (char*)alloc.malloc(10);
    assert_eq((long(ptr) % ALIGNMENT), 0);
    alloc.free(ptr);
}

