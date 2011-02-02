#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"

#include "errors.hpp"

buf_lock_t::buf_lock_t(transaction_t *tx, block_id_t block_id, access_t mode) :
    buf_(co_acquire_block(tx, block_id, mode)), home_thread_(tx->home_thread) { }

buf_lock_t::buf_lock_t(transactor_t& txor, block_id_t block_id, access_t mode) :
    buf_(co_acquire_block(txor.transaction(), block_id, mode)), home_thread_(get_thread_id()) { }

void buf_lock_t::allocate(transactor_t& txor) {
    guarantee(buf_ == NULL);
    txor.transaction()->ensure_thread();
    buf_ = txor.transaction()->allocate();
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
