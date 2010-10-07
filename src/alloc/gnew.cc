#include "alloc/gnew.hpp"
#include "errors.hpp"
#include <new>
#include <stdlib.h>

void* operator new(size_t size) throw(std::bad_alloc) {
#ifndef NDEBUG
    fail("You are using builtin operator new. Please use RethinkDB allocator system instead.");
#else
    // Provide an implementation in case we do this in release mode.
    void *ptr = NULL;
    int res = posix_memalign(&ptr, 64, size);
    if(res != 0)
        throw std::bad_alloc();
    else
        return ptr;
#endif
}

void operator delete(void *p) {
#ifndef NDEBUG
    fail("You are using builtin operator delete, but you somehow failed to trip the warning about "
        "builtin operator new. Maybe you need to add a virtual destructor somewhere, or maybe "
        "you are using the delete operator on something that was allocated with gnew().");
#else
    free(p);
#endif
}

void *_gmalloc(size_t size) {
    void *ptr = NULL;
    int res = posix_memalign((void**)&ptr, 64, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

void _gfree(void *ptr) {
    free(ptr);
}
