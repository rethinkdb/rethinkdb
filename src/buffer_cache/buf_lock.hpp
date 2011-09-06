#ifndef __BUFFER_CACHE_BUF_LOCK_HPP__
#define __BUFFER_CACHE_BUF_LOCK_HPP__

// TODO: get rid of a separate buf_t entirely (that is, have buf_t use RAII).

#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "utils.hpp"

class cond_t;

// A buf_lock_t acquires and holds a buf_t.  Make sure you call
// release() as soon as it's feasible to do so.  The destructor will
// release the buf_t, so don't worry!

// YOU MAY ONLY USE THIS TYPE FROM ITS HOME THREAD.

int get_thread_id();

class buf_lock_t {
public:
    buf_lock_t() : buf_(NULL)
#ifndef NDEBUG
                 , home_thread_(-1)
#endif  // NDEBUG
    { }

    buf_lock_t(transaction_t *txn, block_id_t block_id, access_t mode, cond_t *acquisition_cond = NULL);
    ~buf_lock_t();

    void allocate(transaction_t *txn);

    // Releases the buf.  You can only release once (unless you swap
    // in an unreleased buf_lock_t).
    void release();

    // Releases the buf, if it was acquired.
    void release_if_acquired();

    // Gives up ownership of the buf_t.
    buf_t *give_up_ownership() {
        assert_thread();
        buf_t *tmp = buf_;
        buf_ = NULL;
        return tmp;
    }

    // Gets the buf_t that has been locked.  Don't call release() on it!
    // TODO: Remove buf_t::release, or make it private.
    buf_t *buf() {
        assert_thread();
        return buf_;
    }

    buf_t *operator->() {
        assert_thread();
        return buf_;
    }

    // Swaps this buf_lock_t with another, thus obeying RAII since one
    // buf_lock_t owns a buf_t at a time.
    void swap(buf_lock_t& swapee) {
        assert_thread();
        swapee.assert_thread();
        std::swap(buf_, swapee.buf_);
#ifndef NDEBUG
        std::swap(home_thread_, swapee.home_thread_);
#endif  // NDEBUG
    }

    // Is a buf currently acquired?
    bool is_acquired() const {
        assert_thread();
        return buf_ != NULL;
    }

    void assert_thread() const {
        rassert(home_thread_ == -1 || get_thread_id() == home_thread_);
    }

private:
    // The acquired buffer, or NULL if it has already been released.
    buf_t *buf_;

#ifndef NDEBUG
    // The thread on which the acquired buffer belongs (and must be released on)
    int home_thread_;
#endif  // NDEBUG

    DISABLE_COPYING(buf_lock_t);
};



#endif  // __BUFFER_CACHE_BUF_LOCK_HPP__
