#ifndef __BTREE_BUF_LOCK_HPP__
#define __BTREE_BUF_LOCK_HPP__

// TODO: put this in buffer_cache/.

// TODO: get rid of a separate buf_t entirely (that is, have buf_t use RAII).

#include "buffer_cache/buffer_cache.hpp"

// A buf_lock_t acquires and holds a buf_t.  Make sure you call
// release() as soon as it's feasible to do so.  The destructor will
// release the buf_t, so don't worry!

class buf_lock_t {
public:
    buf_lock_t() : buf_(NULL) { }
    buf_lock_t(transaction_t *tx, block_id_t block_id, access_t mode);
    ~buf_lock_t();

    // Releases the buf.  You can only release once (unless you swap
    // in an unreleased buf_lock_t).
    void release();

    // Gets the buf_t that has been locked.  Don't call release() on it!
    // TODO: Remove buf_t::release, or make it private.
    buf_t *buf() { return buf_; }

    // Swaps this buf_lock_t with another, thus obeying RAII since one
    // buf_lock_t owns a buf_t at a time.
    void swap(buf_lock_t& swapee) {
        buf_t *tmp = buf_;
        buf_ = swapee.buf_;
        swapee.buf_ = tmp;
    }

    // Is a buf currently acquired?
    bool is_acquired() const { return buf_ != NULL; }

private:
    // The acquired buffer, or NULL if it has already been released.
    buf_t *buf_;

    DISABLE_COPYING(buf_lock_t);
};



#endif  // __BTREE_BUF_LOCK_HPP__
