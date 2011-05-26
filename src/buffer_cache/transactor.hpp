#ifndef __BUFFER_CACHE_TRANSACTOR_HPP__
#define __BUFFER_CACHE_TRANSACTOR_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"

// A transactor_t is an RAII thingy for a transaction_t.  You might
// want to rename this (we'd really want transaction_t but that's
// taken).


class transactor_t {
public:
    // HEY: Get rid of these order_token_t::ignore uses, and make
    // people pass them explicitly.

    // This must be used in read mode or (read_mode_outdated_ok)
    transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, order_token_t token);

    transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp, order_token_t token);

    ~transactor_t();

    transaction_t *get() { return transaction_; }
    transaction_t *operator->() { return transaction_; }
private:
    transaction_t *transaction_;

    DISABLE_COPYING(transactor_t);
};




#endif  // __BUFFER_CACHE_TRANSACTOR_HPP__
