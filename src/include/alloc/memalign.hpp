
#ifndef __MEMALIGN_ALLOC_HPP__
#define __MEMALIGN_ALLOC_HPP__

#include <stdlib.h>

template<int alignment = 1024 * 8>
struct memalign_alloc_t {
    void* malloc(size_t size) {
        void *ptr = NULL;
        int res = posix_memalign(&ptr, alignment, size);
        if(res != 0)
            return NULL;
        else
            return ptr;
    }
    
    void free(void* ptr) {
        ::free(ptr);
    }
};

#endif // __MEMALIGN_ALLOC_HPP__

