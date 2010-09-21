#ifndef __CONFIG_ALLOC_HPP__
#define __CONFIG_ALLOC_HPP__

#include "config/args.hpp"
#include "alloc/memalign.hpp"
#ifdef VALGRIND
#include "alloc/malloc.hpp"
#endif

/**
 * The allocator system
 */
typedef memalign_alloc_t<BTREE_BLOCK_SIZE> buffer_alloc_t; // TODO: we need a better allocator
#ifdef VALGRIND
    typedef malloc_alloc_t alloc_t;
#else
    typedef dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > > alloc_t;
#endif

#include "alloc/alloc_mixin.hpp"

#endif /* __CONFIG_ALLOC_HPP__ */
