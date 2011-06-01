#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

transactor_t::transactor_t(cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp) : transaction_(co_begin_transaction(cache, access, expected_change_count, recency_timestamp)) { }

transactor_t::transactor_t(cache_t *cache, access_t access) : transaction_(co_begin_transaction(cache, access, 0, repli_timestamp::invalid)) {
    rassert(is_read_mode(access), "Write transactions should please provide an expected change count.");
}

transactor_t::~transactor_t() {
    if (transaction_) {
        on_thread_t th(transaction_->home_thread());
        guarantee(transaction_ != NULL);
        co_commit_transaction(transaction_);
        transaction_ = NULL;
    }
}
