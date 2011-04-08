#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"

#include "errors.hpp"

buf_lock_t::buf_lock_t(UNUSED const thread_saver_t& saver, transaction_t *tx, block_id_t block_id, access_t mode) :
    home_thread_(tx->home_thread) {
    on_thread_t thread_switcher(tx->home_thread);
    buf_ = tx->acquire(block_id, mode);
}

buf_lock_t::buf_lock_t(UNUSED const thread_saver_t& saver, transactor_t& txor, block_id_t block_id, access_t mode) :
    home_thread_(get_thread_id()) {
    on_thread_t thread_switcher(txor.transaction()->home_thread);
    buf_ = txor.transaction()->acquire(block_id, mode);
}

void buf_lock_t::allocate(const thread_saver_t& saver, transactor_t& txor) {
    guarantee(buf_ == NULL);
    txor.transaction()->ensure_thread(saver);
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
