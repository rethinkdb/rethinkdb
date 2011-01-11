#include "btree/buf_lock.hpp"

#include "btree/co_functions.hpp"
#include "errors.hpp"

buf_lock_t::buf_lock_t(transaction_t *tx, block_id_t block_id, access_t mode) : buf_(co_acquire_transaction(tx, block_id, mode)) { }

buf_lock_t::~buf_lock_t() {
    if (buf_) {
        buf_->release();
        buf_ = NULL;
    }
}

void buf_lock_t::release() {
    guarantee(buf_ != NULL);
    buf_->release();
    buf_ = NULL;
}
