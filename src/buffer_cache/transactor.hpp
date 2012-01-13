#ifndef __BUFFER_CACHE_TRANSACTOR_HPP__
#define __BUFFER_CACHE_TRANSACTOR_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"

// A transactor_t is an RAII thingy for a transaction_t.  You might
// want to rename this (we'd really want transaction_t but that's
// taken).

class sequence_group_t;

class transactor_t {
public:
    // This must be used in read mode or (read_mode_outdated_ok)
    transactor_t(cache_t *cache, sequence_group_t *seq_group, access_t access);

    transactor_t(cache_t *cache, sequence_group_t *seq_group, access_t access, int expected_change_count, repli_timestamp recency_timestamp);

    ~transactor_t();

    transaction_t *get() { return transaction_; }
    transaction_t *operator->() { return transaction_; }
private:
    transaction_t *transaction_;

    DISABLE_COPYING(transactor_t);
};




#endif  // __BUFFER_CACHE_TRANSACTOR_HPP__
