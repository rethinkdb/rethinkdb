#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"

#include "errors.hpp"

buf_lock_t::buf_lock_t(const thread_saver_t& saver, transactor_t& txor, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) :
    buf_(co_acquire_block(saver, txor.get(), block_id, mode, acquisition_cond)), home_thread_(get_thread_id()) { }

buf_lock_t::buf_lock_t(transactor_t& txor, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) : buf_(NULL), home_thread_(get_thread_id()) {
    txor->assert_thread();
    thread_saver_t saver;
    buf_ = co_acquire_block(saver, txor.get(), block_id, mode, acquisition_cond);
}

void buf_lock_t::allocate(const thread_saver_t& saver, transactor_t& txor) {
    guarantee(buf_ == NULL);
    txor->ensure_thread(saver);
    buf_ = txor->allocate();
    home_thread_ = get_thread_id();
}

void buf_lock_t::allocate(transactor_t& txor) {
    txor->assert_thread();
    thread_saver_t saver;
    allocate(saver, txor);
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
