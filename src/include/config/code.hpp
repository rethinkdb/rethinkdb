
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
#include "buffer_cache/stats.hpp"
#include "buffer_cache/mirrored.hpp"
#include "buffer_cache/page_map/unlocked_hash_map.hpp"
#include "buffer_cache/page_repl/none.hpp"
#include "buffer_cache/writeback/immediate.hpp"
#include "buffer_cache/concurrency/bkl.hpp"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/array_node.hpp"

/**
 * Code configuration - instantiating various templated classes.
 */
struct standard_config_t {
    // IO buffer
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;
    
    // IO syscalls
    typedef posix_io_calls_t iocalls_t;
    
    // FSMs
    typedef conn_fsm<standard_config_t> conn_fsm_t;
    typedef intrusive_list_t<conn_fsm_t> fsm_list_t;
    typedef btree_fsm<standard_config_t> btree_fsm_t;

    // Serializer
    typedef in_place_serializer_t<standard_config_t> serializer_t;

    // Caching
    typedef memalign_alloc_t<BTREE_BLOCK_SIZE> buffer_alloc_t;
    typedef unlocked_hash_map_t<standard_config_t> page_map_t;
    typedef page_repl_none_t<standard_config_t> page_repl_t;
    typedef buffer_cache_bkl_t<standard_config_t> concurrency_t;
    typedef immediate_writeback_t<standard_config_t> writeback_t;
    typedef mirrored_cache_t<standard_config_t> cache_impl_t;
    
    //typedef fallthrough_cache_t<standard_config_t> cache_impl_t;
#ifdef NDEBUG
    typedef cache_impl_t cache_t;
#else
    typedef cache_stats_t<cache_impl_t> cache_t;
#endif

    // BTree
    typedef btree_admin<standard_config_t> btree_admin_t;
    typedef array_node_t<serializer_t::block_id_t> node_t;
    typedef btree_get_fsm<standard_config_t> btree_get_fsm_t;
    typedef btree_set_fsm<standard_config_t> btree_set_fsm_t;

    // Request handler
    typedef request_handler_t<standard_config_t> req_handler_t;

    // Small object allocator
    typedef object_static_alloc_t<
        dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > >,
        iocb, conn_fsm_t, iobuf_t, btree_get_fsm_t, btree_set_fsm_t> alloc_t;
};

typedef standard_config_t code_config_t;

#endif // __CONFIG_CODE_H__

