
#ifndef __MEMALIGN_ALLOC_HPP__
#define __MEMALIGN_ALLOC_HPP__

#include <stdlib.h>
#include <string.h>

template<int alignment = 1024 * 8>
struct memalign_alloc_t {
    memalign_alloc_t() {}
    explicit memalign_alloc_t(size_t size) {} // For compatibility
    
    void* malloc(size_t size) {
        void *ptr = NULL;
        int res = posix_memalign(&ptr, alignment, size);
        if(res != 0) {
            return NULL;
        } else {
#if defined(VALGRIND) || !defined(NDEBUG)
            // Fill the buffer with garbage in debug mode so valgrind doesn't complain, and to help
            // catch uninitialized memory errors.
            memset(ptr, 0xBD, size);
#endif
            return ptr;
        }
    }
    
    void free(void* ptr) {
        ::free(ptr);
    }
};

#endif // __MEMALIGN_ALLOC_HPP__

