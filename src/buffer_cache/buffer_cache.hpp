
#ifndef __BUFFER_CACHE_HPP__
#define __BUFFER_CACHE_HPP__

#ifndef MOCK_BUFFER_CACHE

/* Use the mirrored cache */

#include "serializer/serializer.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/mirrored/concurrency/rwi_conc.hpp"
#include "buffer_cache/mirrored/page_map/array.hpp"
#include "buffer_cache/mirrored/page_repl/page_repl_random.hpp"
#include "buffer_cache/mirrored/writeback/writeback.hpp"

struct standard_mc_config_t {
    
    typedef ::serializer_t serializer_t;
    
    typedef rwi_conc_t<standard_mc_config_t> concurrency_t;
    typedef array_map_t<standard_mc_config_t> page_map_t;
    typedef page_repl_random_t<standard_mc_config_t> page_repl_t;
    typedef writeback_tmpl_t<standard_mc_config_t> writeback_t;
};

typedef mc_cache_t<standard_mc_config_t> cache_t;

#else

TODO: when we have a mock buffer cache, it goes here

#endif

/* Move elements of chosen cache into global namespace */

typedef cache_t::buf_t buf_t;
typedef cache_t::transaction_t transaction_t;
typedef cache_t::block_available_callback_t block_available_callback_t;
typedef cache_t::transaction_begin_callback_t transaction_begin_callback_t;
typedef cache_t::transaction_commit_callback_t transaction_commit_callback_t;

#endif /* __BUFFER_CACHE_HPP__ */
