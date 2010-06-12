
#ifndef __ALLOC_MIXIN_HPP__
#define __ALLOC_MIXIN_HPP__

#include <vector>

/**
 * Allocation system mixin class.  Classes that derive from this will
 * use a stackable allocator returned by the accessor.
 */
template <class accessor_t, class type_t>
class alloc_mixin_t {
public:
    static void *operator new(size_t);
    static void operator delete(void *);
};

/**
 * Allocator system mixin class requiring the user to specify an allocator to
 * the new() call; this is then saved automatically for the subsequent delete().
 */
template <class alloc_t>
class alloc_runtime_mixin_t {
public:
    static void *operator new(size_t, alloc_t *);
    static void operator delete(void *, alloc_t *);
    static void operator delete(void *);
};

/**
 * A helper accessor that creates small object allocators (per-core,
 * per object) automagically.
 */
template <class alloc_t>
class tls_small_obj_alloc_accessor {
public:
    // get_alloc will create a thread local version of the allocator
    // for every type
    template <class type_t>
    static alloc_t* get_alloc() {
        static __thread alloc_t *alloc = NULL;
        if(!allocs)
            allocs = new std::vector<alloc_t*>;

        if(!alloc) {
            alloc = new alloc_t(sizeof(type_t));
            allocs->push_back(alloc);
        }
        return alloc;
    }
    
    static __thread std::vector<alloc_t*> *allocs;
};

#include "alloc/alloc_mixin_impl.hpp"

#endif /* __ALLOC_MIXIN_HPP__ */
