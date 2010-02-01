
#ifndef __CONFIG_CODE_H__
#define __CONFIG_CODE_H__

#include "corefwd.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/object_static.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "config/args.hpp"
#include "arch/io.hpp"
#include "serializer/in_place.hpp"
#include "buffer_cache/fallthrough.hpp"
#include "btree/btree.hpp"
#include "btree/array_node.hpp"

/**
 * Code configuration - instantiating various templated classes.
 */
struct standard_config_t {
    // IO buffer
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;
    
    // IO syscalls
    typedef posix_io_calls_t iocalls_t;
    
    // FSM
    typedef fsm_state_t<standard_config_t> fsm_t;
    typedef intrusive_list_t<fsm_t> fsm_list_t;

    // Serializer
    typedef in_place_serializer_t<standard_config_t> serializer_t;

    // Cache
    typedef fallthrough_cache_t<standard_config_t> cache_t;

    // BTree
    typedef array_node_t<serializer_t::block_id_t> node_t;
    typedef btree<standard_config_t> btree_t;

    // Small object allocator
    typedef object_static_alloc_t<
        dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > >,
        iocb, fsm_t, iobuf_t> alloc_t;
};

typedef standard_config_t code_config_t;

#endif // __CONFIG_CODE_H__

