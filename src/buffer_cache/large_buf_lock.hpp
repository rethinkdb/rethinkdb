#ifndef __BUFFER_CACHE_LARGE_BUF_LOCK_HPP__
#define __BUFFER_CACHE_LARGE_BUF_LOCK_HPP__

#include "buffer_cache/large_buf.hpp"

// TODO: Ensure that we _never_ call release without acquire or allocate.

// TODO: Okay, why we should now just use scoped_ptr.

class large_buf_lock_t {
public:
    large_buf_lock_t() : lv_(NULL) { }
    // lv _must_ end up being acquired quickly.  TODO don't require this.
    large_buf_lock_t(large_buf_t *lv) : lv_(lv) { }
    ~large_buf_lock_t() {
        delete lv_;
        lv_ = NULL;
    }

    void set(large_buf_t *other) {
        assert(lv_ == NULL);
        lv_ = other;
    }

    void reset(large_buf_t *other = NULL) {
        delete lv_;
        lv_ = NULL;
        lv_ = other;
    }


    void swap(large_buf_lock_t& other) {
        large_buf_t *tmp = other.lv_;
        other.lv_ = lv_;
        lv_ = tmp;
    }

    // We'd like to rename this to get(), but that's not important.
    large_buf_t *lv() { return lv_; }

    large_buf_t *operator->() { return lv_; }

    bool has_lv() const { return lv_ != NULL; }

private:
    large_buf_t *lv_;

    DISABLE_COPYING(large_buf_lock_t);
};



#endif  // __BUFFER_CACHE_LARGE_BUF_LOCK_HPP__
