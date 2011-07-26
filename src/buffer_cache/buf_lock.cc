#include "buffer_cache/buf_lock.hpp"

#include "arch/core.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/co_functions.hpp"
#include "errors.hpp"

buf_lock_t::buf_lock_t(transaction_t *txn, block_id_t block_id, access_t mode, cond_t *acquisition_cond) :
    buf_(co_acquire_block(txn, block_id, mode, acquisition_cond)), home_thread_(get_thread_id()) { }

void buf_lock_t::allocate(transaction_t *txn) {
    txn->assert_thread();
    guarantee(buf_ == NULL);
    buf_ = txn->allocate();
    home_thread_ = get_thread_id();
}


buf_lock_t::~buf_lock_t() {
    release_if_acquired();
}

void buf_lock_t::release() {
    guarantee(buf_ != NULL);
    on_thread_t thread_switcher(home_thread_);
    buf_->release();
    buf_ = NULL;
}

void buf_lock_t::release_if_acquired() {
    if (buf_)
        release();
}
