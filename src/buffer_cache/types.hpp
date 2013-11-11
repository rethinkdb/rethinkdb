// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_TYPES_HPP_
#define BUFFER_CACHE_TYPES_HPP_

#include "buffer_cache/general_types.hpp"

// Keep this part below synced up with buffer_cache.hpp.

class mc_cache_t;
class mc_buf_lock_t;
class mc_transaction_t;
class mc_cache_account_t;

#if !defined(VALGRIND) && !defined(NDEBUG)

template <class inner_cache_type> class scc_cache_t;
template <class inner_cache_type> class scc_buf_lock_t;
template <class inner_cache_type> class scc_transaction_t;

typedef scc_cache_t<mc_cache_t> cache_t;
typedef scc_buf_lock_t<mc_cache_t> buf_lock_t;
typedef scc_transaction_t<mc_cache_t> transaction_t;
typedef mc_cache_account_t cache_account_t;

#else  // !defined(VALGRIND) && !defined(NDEBUG)

// scc_cache_t is way too slow under valgrind and makes automated
// tests run forever.
typedef mc_cache_t cache_t;
typedef mc_buf_lock_t buf_lock_t;
typedef mc_transaction_t transaction_t;
typedef mc_cache_account_t cache_account_t;

#endif  // !defined(VALGRIND) && !defined(NDEBUG)


#endif /* BUFFER_CACHE_TYPES_HPP_ */
