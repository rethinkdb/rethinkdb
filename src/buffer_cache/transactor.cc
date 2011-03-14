#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

transactor_t::transactor_t(cache_t *cache, access_t access, repli_timestamp recency_timestamp) : transaction_(co_begin_transaction(cache, access, recency_timestamp)) { }

transactor_t::~transactor_t() {
    if (transaction_) {
        co_commit_transaction(transaction_);
        transaction_ = NULL;
    }
}

void transactor_t::commit() {
    guarantee(transaction_ != NULL);
    co_commit_transaction(transaction_);
    transaction_ = NULL;
}
