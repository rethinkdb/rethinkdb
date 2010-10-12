
#ifndef __BUFFER_CACHE_HPP__
#define __BUFFER_CACHE_HPP__

/* Choose our cache */

#ifndef MOCK_CACHE_CHECK

#include "serializer/serializer.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/mirrored/concurrency/rwi_conc.hpp"
#include "buffer_cache/mirrored/page_map.hpp"
#include "buffer_cache/mirrored/page_repl/page_repl_random.hpp"
#include "buffer_cache/mirrored/writeback/writeback.hpp"
#include "buffer_cache/mirrored/free_list.hpp"

struct standard_mc_config_t {
    
    typedef ::serializer_t serializer_t;
    
    typedef rwi_conc_t<standard_mc_config_t> concurrency_t;
    typedef array_map_t<standard_mc_config_t> page_map_t;
    typedef page_repl_random_t<standard_mc_config_t> page_repl_t;
    typedef writeback_tmpl_t<standard_mc_config_t> writeback_t;
    typedef array_free_list_t<standard_mc_config_t> free_list_t;
};

#include "buffer_cache/semantic_checking.hpp"

#if !defined(VALGRIND) && !defined(NDEBUG)
// scc_cache_t is way too slow under valgrind and makes automated
// tests run forever.
typedef scc_cache_t<mc_cache_t<standard_mc_config_t> > cache_t;
#else
typedef mc_cache_t<standard_mc_config_t> cache_t;
#endif

#else

#include "buffer_cache/mock.hpp"
typedef mock_cache_t cache_t;

#endif /* MOCK_CACHE */

/* Move elements of chosen cache into global namespace */

typedef cache_t::buf_t buf_t;
typedef cache_t::transaction_t transaction_t;
typedef cache_t::block_available_callback_t block_available_callback_t;
typedef cache_t::transaction_begin_callback_t transaction_begin_callback_t;
typedef cache_t::transaction_commit_callback_t transaction_commit_callback_t;

#endif /* __BUFFER_CACHE_HPP__ */
