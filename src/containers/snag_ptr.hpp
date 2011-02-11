#ifndef __CONTAINERS_SNAG_PTR_HPP__
#define __CONTAINERS_SNAG_PTR_HPP__

#include "concurrency/cond_var.hpp"

class snag_pointee_mixin_t {
public:
    snag_pointee_mixin_t() : reference_count_(0), cond_() { }

    template <class T>
    friend class snag_ptr_t;



protected:
    void wait_until_ready_to_delete() {
        if (reference_count_ > 0) {
            cond_.wait();
        }
    }
    ~snag_pointee_mixin_t() {
        wait_until_ready_to_delete();
    }
private:
    void incr_reference_count() {
        ++ reference_count_;
    }

    void decr_reference_count() {
        -- reference_count_;
        if (reference_count_ == 0) {
            cond_.pulse();
        }
    }


    size_t reference_count_;
    cond_t cond_;
    DISABLE_COPYING(snag_pointee_mixin_t);
};


template <class T>
class snag_ptr_t {
public:
    snag_ptr_t(const snag_ptr_t& other)
        : ptr_(other.ptr_) {
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::incr_reference_count, ptr_));
        }
    }

    snag_ptr_t() : ptr_(NULL) { }

    operator bool() const { return ptr_ != NULL; }

    explicit snag_ptr_t(T& ptr) : ptr_(&ptr) {
        do_on_thread(ptr_->home_thread, boost::bind(&T::incr_reference_count, ptr_));
    }

    ~snag_ptr_t() {
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::decr_reference_count, ptr_));
        }
    }

    void reset(T *ptr = NULL) {
        if (ptr) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::incr_reference_count, ptr));
        }
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::decr_reference_count, ptr_));
        }
        ptr_ = ptr;
    }

    T *get() const { return ptr_; }
    T *operator->() const { return ptr_; }
private:
    T *ptr_;
};

#endif  // __CONTAINERS_SNAG_PTR_HPP__
