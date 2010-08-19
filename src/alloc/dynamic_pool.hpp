
#ifndef __DYNAMIC_POOL_ALLOC_HPP__
#define __DYNAMIC_POOL_ALLOC_HPP__

#include <math.h>
#include "config/args.hpp"
#include "utils.hpp"

// TODO: We double the size of the allocator every time, which means
// we can use half the RAM or so until the super_alloc allocator
// request will fail. If we get to this point we should adjust the
// allocation factor (but it's not clear this will ever happen in
// practice).

template <class super_alloc_t>
struct dynamic_pool_alloc_t {
    explicit dynamic_pool_alloc_t(size_t _object_size)
        : nallocs(1), smallest_free(0), object_size(_object_size)
        {
            check("Dynamic pool configuration error", DYNAMIC_POOL_MAX_ALLOCS < nallocs);
            allocs[0] = gnew<super_alloc_t>(compute_alloc_nobjects(0), object_size);
            check("Could not allocate memory in dynamic pool", allocs[0] == NULL);
        }
    ~dynamic_pool_alloc_t() {
        for(unsigned int i = 0; i < nallocs; i++) {
            gdelete(allocs[i]);
        }
    }

    void *malloc(size_t size) {
        // Try to allocate from the smallest allocator that was last free
        void *ptr = allocs[smallest_free]->malloc(size);
        // If we couldn't allocate memory, we have to do more work
        if(!ptr) {
            // First, try to go through existing allocators
            for(unsigned int i = smallest_free; i < nallocs; i++) {
                ptr = allocs[i]->malloc(size);
                if(ptr) {
                    smallest_free = i;
                    break;
                }
            }
            // If we still couldn't allocate, create a new allocator (if we can)
            if(!ptr && nallocs + 1 <= DYNAMIC_POOL_MAX_ALLOCS) {
                nallocs++;
                smallest_free = nallocs - 1;
                allocs[smallest_free] =
                    gnew<super_alloc_t>(compute_alloc_nobjects(smallest_free),
                                        object_size);
                ptr = allocs[smallest_free]->malloc(size);
            }
        }
        return ptr;
    }

    void free(void *ptr) {
        // We have an option of prepending an allocator pointer to
        // each object, making deallocation constant time. However,
        // this messes with user's exepctations of alignment and
        // increases memory consumption. Since the chain of allocators
        // is expected to be very short (probably no more than 10), a
        // loop is an easier solution for now.
#ifndef NDEBUG
        bool ptr_freed = false;
#endif
        for(unsigned int i = 0; i < nallocs; i++) {
            if(allocs[i]->in_range(ptr)) {
                allocs[i]->free(ptr);
                if(i < smallest_free) {
                    smallest_free = i;
                }
#ifndef NDEBUG
                ptr_freed = true;
#endif
                break;
            }
        }
        assert(ptr_freed);
    }

    // This function should be called periodically (probably on
    // timer), if we want the allocator to release unused memory back
    // to the system. Otherwise, the allocator will use as much memory
    // as was required during peak utilization. Note that this isn't
    // strictly a garbage collector, as the garbage doesn't
    // accumilate.
    int gc() {
        int blocks_reclaimed = 0;
        int mem_reclaimed = 0;

        for(int i = nallocs - 1; i > 0; i--) {
            if(allocs[i]->empty()) {
                gdelete(allocs[i]);
                allocs[i] = NULL;
                nallocs--;
                blocks_reclaimed++;
                mem_reclaimed += compute_alloc_nobjects(i) * object_size;
            } else {
                break;
            }
        }
        if(smallest_free > nallocs - 1)
            smallest_free = nallocs - 1;

        // TODO: convert this to a logging infrustructure (when it's in place)
#ifndef NDEBUG
        if(blocks_reclaimed > 0) {
            printf("gc (%dB in %d blocks)\n", mem_reclaimed, blocks_reclaimed);
        }
#endif
        return blocks_reclaimed;
    }

private:
    super_alloc_t* allocs[DYNAMIC_POOL_MAX_ALLOCS];
    unsigned int nallocs;
    unsigned int smallest_free;
    size_t object_size;

    unsigned int compute_alloc_nobjects(int alloc) {
        int n = DYNAMIC_POOL_INITIAL_NOBJECTS * pow(2, alloc);
        return n;
    }
};

#endif // __DYNAMIC_POOL_ALLOC_HPP__

