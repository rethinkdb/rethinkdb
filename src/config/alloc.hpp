#ifndef __CONFIG_ALLOC_HPP__
#define __CONFIG_ALLOC_HPP__

#include "config/args.hpp"
#include "alloc/malloc.hpp"
#include "alloc/memalign.hpp"
#ifdef VALGRIND
#include "alloc/malloc.hpp"
#else
#include "alloc/gnew.hpp"
#include "alloc/stats.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#endif

/**
 * The allocator system
 */
#ifdef VALGRIND
    typedef malloc_alloc_t alloc_t;
#else
    typedef dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > > alloc_t;
#endif

#include "alloc/alloc_mixin.hpp"

#endif /* __CONFIG_ALLOC_HPP__ */
