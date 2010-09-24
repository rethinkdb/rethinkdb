
#ifndef __ALLOC_MIXIN_HPP__
#define __ALLOC_MIXIN_HPP__

#include <vector>
#include "errors.hpp"
#include "alloc/gnew.hpp"

// TODO: add support for object lifetime histogram

/**
 * Allocation system mixin class.  Classes that derive from this will
 * use a stackable allocator returned by the accessor.
 */
template <class accessor_t, class type_t>
class alloc_mixin_t {
public:
    static void *operator new(size_t);
    static void operator delete(void *ptr);
};

/**
 * A helper accessor that creates small object allocators (per-core,
 * per object) automagically.
 */
template <class alloc_t>
class tls_small_obj_alloc_accessor {
public:
    typedef std::vector<alloc_t*, gnew_alloc<alloc_t*> > alloc_vector_t;
public:
    // get_alloc will create a thread local version of the allocator
    // for every type
    template <class type_t>
    static alloc_t* get_alloc() {
        static __thread alloc_t *alloc = NULL;
        if(!allocs_tl)
            allocs_tl = gnew<alloc_vector_t>();

        if(!alloc) {
#ifdef NDEBUG
            alloc = gnew<alloc_t>(sizeof(type_t));
#else
            // In debug mode, we need space for an extra pointer
            alloc = gnew<alloc_t>(sizeof(void*) + sizeof(type_t));
#endif
            allocs_tl->push_back(alloc);
        }
        return alloc;
    }
    
    static __thread alloc_vector_t *allocs_tl;
};

/* A helper object that just gives access to a single global allocator.
Warning: Only use this if the underlying allocator is thread-safe! */
template<class alloc_t>
struct cross_thread_alloc_accessor_t {
    
    template<class type_t>
    static alloc_t *get_alloc() {
        static alloc_t *alloc = NULL;
        if (!alloc) alloc = gnew<alloc_t>(sizeof(type_t));
        return alloc;
    }
};

#include "alloc/alloc_mixin.tcc"

#endif /* __ALLOC_MIXIN_HPP__ */
