
#ifndef __ALLOC_MIXIN_HPP__
#define __ALLOC_MIXIN_HPP__

/**
 * Allocation system mixin class.  Classes that derive from this will use
 * a stackable allocator set in epoll_handler() for their new() and delete().
 */
template <class alloc_t>
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

#include "alloc/alloc_mixin_impl.hpp"

#endif /* __ALLOC_MIXIN_HPP__ */
