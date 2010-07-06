
#ifndef __MALLOC_ALLOC_HPP__
#define __MALLOC_ALLOC_HPP__

#include <stdlib.h>

struct malloc_alloc_t {
    // Standard constructor
    malloc_alloc_t() {}
    
    // We really don't need to take size here, but some super
    // allocators expect a constructor like this
    explicit malloc_alloc_t(size_t _size) {}

    void gc() {}
    
    void* malloc(size_t size) {
        void *ptr = ::malloc(size);
#if defined(VALGRIND) || !defined(NDEBUG)
            // Fill the buffer with garbage in debug mode so valgrind doesn't complain, and to help
            // catch uninitialized memory errors.
            memset(ptr, 0xBD, size);
#endif
        return ptr;
    }
    
    void free(void* ptr) {
        ::free(ptr);
    }
};

#endif // __MALLOC_ALLOC_HPP__

