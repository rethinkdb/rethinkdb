#ifndef __BUFFER_CACHE_TRANSACTOR_HPP__
#define __BUFFER_CACHE_TRANSACTOR_HPP__

#include "buffer_cache/buffer_cache.hpp"

// A transactor_t is an RAII thingy for a transaction_t.  You might
// want to rename this (we'd really want transaction_t but that's
// taken).


class transactor_t {
public:
    transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, repli_timestamp recency_timestamp);
    transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp);
    ~transactor_t();

    transaction_t *transaction() { return transaction_; }
    transaction_t *operator->() { return transaction_; }
    void commit(const thread_saver_t& saver);
private:
    transaction_t *transaction_;

    DISABLE_COPYING(transactor_t);
};




#endif  // __BUFFER_CACHE_TRANSACTOR_HPP__
