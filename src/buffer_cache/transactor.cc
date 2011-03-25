#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

transactor_t::transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp) : transaction_(co_begin_transaction(saver, cache, access, expected_change_count, recency_timestamp)) { }

transactor_t::transactor_t(const thread_saver_t& saver, cache_t *cache, access_t access, repli_timestamp recency_timestamp) : transaction_(co_begin_transaction(saver, cache, access, 0, recency_timestamp)) {
    rassert (access != rwi_write, "Write transactions should please provide an expected change count.");
}

transactor_t::~transactor_t() {
    if (transaction_) {
        thread_saver_t saver;
        commit(saver);
    }
}

void transactor_t::commit(const thread_saver_t& saver) {
    guarantee(transaction_ != NULL);
    co_commit_transaction(saver, transaction_);
    transaction_ = NULL;
}
