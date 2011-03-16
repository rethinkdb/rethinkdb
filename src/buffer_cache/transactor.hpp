#ifndef __BUFFER_CACHE_TRANSACTOR_HPP__
#define __BUFFER_CACHE_TRANSACTOR_HPP__

#include "buffer_cache/buffer_cache.hpp"

// A transactor_t is an RAII thingy for a transaction_t.  You might
// want to rename this (we'd really want transaction_t but that's
// taken).


class transactor_t {
public:
    transactor_t(cache_t *cache, access_t access);
    transactor_t(cache_t *cache, access_t access, int expected_change_count);
    ~transactor_t();

    transaction_t *transaction() { return transaction_; }
    void commit();
private:
    transaction_t *transaction_;

    DISABLE_COPYING(transactor_t);
};




#endif  // __BUFFER_CACHE_TRANSACTOR_HPP__
