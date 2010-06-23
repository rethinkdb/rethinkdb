
#ifndef __CONFIG_CODE_H__
#define __CONFIG_CODE_H__

#include "corefwd.hpp"
#include "alloc/memalign.hpp"
#ifdef VALGRIND
#include "alloc/malloc.hpp"
#endif

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
    typedef memalign_alloc_t<BTREE_BLOCK_SIZE> buffer_alloc_t; // TODO: we need a better allocator
    typedef unlocked_hash_map_t<standard_config_t> page_map_t;
    typedef page_repl_none_t<standard_config_t> page_repl_t;
    typedef writeback_tmpl_t<standard_config_t> writeback_t;
    typedef rwi_conc_t<standard_config_t> concurrency_t;
    typedef mirrored_cache_t<standard_config_t> cache_impl_t;
    
    //typedef fallthrough_cache_t<standard_config_t> cache_impl_t;
#ifdef NDEBUG
    typedef cache_impl_t cache_t;
#else
    typedef cache_impl_t cache_t;
    //typedef cache_stats_t<cache_impl_t> cache_t; TODO(NNW): Update API.
#endif
    typedef buf<standard_config_t> buf_t;
    typedef transaction<standard_config_t> transaction_t;

    // BTree
    typedef btree_admin<standard_config_t> btree_admin_t;
    typedef array_node_t<off64_t> node_t;
    typedef btree_get_fsm<standard_config_t> btree_get_fsm_t;
    typedef btree_set_fsm<standard_config_t> btree_set_fsm_t;

    // Request handler
    typedef request_handler_t<standard_config_t> req_handler_t;
    typedef request<standard_config_t> request_t;

    // Small object allocator
#ifdef VALGRIND
    typedef malloc_alloc_t alloc_t;
#else
    typedef dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > > alloc_t;
#endif
};

typedef standard_config_t code_config_t;

#endif // __CONFIG_CODE_H__

