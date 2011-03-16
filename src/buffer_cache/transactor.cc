#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

transactor_t::transactor_t(cache_t *cache, access_t access, int expected_change_count) : transaction_(co_begin_transaction(cache, access, expected_change_count)) { }
transactor_t::transactor_t(cache_t *cache, access_t access) : transaction_(co_begin_transaction(cache, access, 0)) {
    rassert (access != rwi_write, "Write transactions should please provide an expected change count.");
}

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
