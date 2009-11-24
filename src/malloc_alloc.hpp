
#ifndef __MALLOC_ALLOC_HPP__
#define __MALLOC_ALLOC_HPP__

#include <stdlib.h>

struct malloc_alloc_t {
    void* malloc(size_t size) {
        return ::malloc(size);
    }
    
    void free(void* ptr) {
        ::free(ptr);
    }
};

#endif // __MALLOC_ALLOC_HPP__

