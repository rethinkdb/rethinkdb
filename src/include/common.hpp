
#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include "config.hpp"
#include "btree/btree.hpp"
#include "btree/array_node.hpp"
#include "buffer_cache/volatile.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/object_static.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "arch/io_calls.hpp"

/**
 * Define the btree
 */
// TODO: This is *VERY* not thread safe
typedef volatile_cache_t rethink_cache_t;
typedef btree<array_node_t<rethink_cache_t::block_id_t>, rethink_cache_t> rethink_tree_t;

/**
 * Define the IO buffer allocator
 */
// TODO: add a checking allocator (check if malloc returns NULL)
typedef dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > > iobuf_alloc_t;

/**
 * Define the state machine
 */
template<class io_calls_t, class alloc_t>
struct fsm_state_t;

typedef fsm_state_t<posix_io_calls_t, iobuf_alloc_t> rethink_fsm_t;

/**
 * Define the small object allocator
 */
// TODO: add a checking allocator (check if malloc returns NULL)
struct iocb;
typedef object_static_alloc_t<
    dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > >,
    iocb, rethink_fsm_t> small_obj_alloc_t;

#endif // __COMMON_HPP__

