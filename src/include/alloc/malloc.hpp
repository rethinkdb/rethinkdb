
#ifndef __MALLOC_ALLOC_HPP__
#define __MALLOC_ALLOC_HPP__

#include <stdlib.h>

struct malloc_alloc_t {
    // Standard constructor
    malloc_alloc_t() {}
    
    // We really don't need to take size here, but some super
    // allocators expect a constructor like this
    malloc_alloc_t(size_t _size) {}
    
    void* malloc(size_t size) {
        return ::malloc(size);
    }
    
    void free(void* ptr) {
        ::free(ptr);
    }
};

#endif // __MALLOC_ALLOC_HPP__

