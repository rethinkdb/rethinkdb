
#ifndef __BUFFER_CACHE_HPP__
#define __BUFFER_CACHE_HPP__

/* Choose our cache */

#ifndef MOCK_CACHE_CHECK

#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/semantic_checking.hpp"

#if !defined(VALGRIND) && !defined(NDEBUG)
// scc_cache_t is way too slow under valgrind and makes automated
// tests run forever.
typedef scc_cache_t<mc_cache_t > cache_t;
#else
typedef mc_cache_t cache_t;
#endif  // !defined(VALGRIND) && !defined(NDEBUG)

#else

#include "buffer_cache/mock.hpp"
#include "buffer_cache/semantic_checking.hpp"
typedef scc_cache_t<mock_cache_t> cache_t;

#endif // MOCK_CACHE_CHECK

/* Move elements of chosen cache into global namespace */

#ifndef CUSTOM_BUF_TYPE
typedef cache_t::buf_t buf_t;
#endif  // CUSTOM_BUF_TYPE
typedef cache_t::transaction_t transaction_t;
typedef cache_t::block_available_callback_t block_available_callback_t;
typedef cache_t::transaction_begin_callback_t transaction_begin_callback_t;
typedef cache_t::transaction_commit_callback_t transaction_commit_callback_t;

#endif /* __BUFFER_CACHE_HPP__ */
