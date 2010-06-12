
#ifndef __ALLOC_STATS_HPP__
#define __ALLOC_STATS_HPP__

#include <stdio.h>
#include "utils.hpp"

template <class super_alloc_t>
struct alloc_stats_t : public super_alloc_t {
    // A general purpose constructor
    alloc_stats_t() : nallocs(0) {}
    alloc_stats_t(size_t) : nallocs(0) {}
    // Constructor version for the pool allocator
    alloc_stats_t(size_t nobjects, size_t object_size)
        : nallocs(0), super_alloc_t(nobjects, object_size)
        {}
    ~alloc_stats_t() {
        check("Memory leak detected", nallocs > 0);
        check("Some objects were freed twice", nallocs < 0);
    }
    
    void* malloc(size_t size) {
        void *ptr = super_alloc_t::malloc(size);
        if(ptr)
            nallocs++;
        return ptr;
    }
    
    void free(void* ptr) {
        super_alloc_t::free(ptr);
        nallocs--;
    }

    int gc() {} // For interface symmetry

    bool empty() {
        return nallocs == 0;
    }

private:
    int nallocs;
};

#endif // __ALLOC_STATS_HPP__

