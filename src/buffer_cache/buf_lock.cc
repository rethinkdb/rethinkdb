#include "buffer_cache/buf_lock.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "buffer_cache/buffer_cache.hpp"

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, cond_t *acquisition_cond) {
    transaction->assert_thread();
    buf_t *value = transaction->acquire(block_id, mode, acquisition_cond ? boost::bind(&cond_t::pulse, acquisition_cond) : boost::function<void()>());
    rassert(value);
    return value;
}

buf_lock_t::buf_lock_t(transaction_t *txn, block_id_t block_id, access_t mode, cond_t *acquisition_cond) :
    buf_(co_acquire_block(txn, block_id, mode, acquisition_cond))
#ifndef NDEBUG
    , home_thread_(get_thread_id())
#endif  // NDEBUG
{
    txn->assert_thread();
}
void buf_lock_t::allocate(transaction_t *txn) {
    assert_thread();
#ifndef NDEBUG
    home_thread_ = get_thread_id();
#endif  // NDEBUG
    guarantee(buf_ == NULL);
    txn->assert_thread();
    buf_ = txn->allocate();
}


buf_lock_t::~buf_lock_t() {
    assert_thread();
    release_if_acquired();
}

void buf_lock_t::release() {
    assert_thread();
    guarantee(buf_ != NULL);
    buf_->release();
    buf_ = NULL;
}

void buf_lock_t::release_if_acquired() {
    assert_thread();
    if (buf_)
        release();
}
