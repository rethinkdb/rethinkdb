#ifndef __CONTAINERS_SNAG_PTR_HPP__
#define __CONTAINERS_SNAG_PTR_HPP__

#include "concurrency/cond_var.hpp"

// A snag_pointee must call wait_until_ready_to_delete.
class snag_pointee_mixin_t {
public:
    snag_pointee_mixin_t() : reference_count_(0), lost_all_references_cond_(), ready_to_delete_(false) { }

    template <class T>
    friend class snag_ptr_t;

protected:
    void wait_until_ready_to_delete() {
        if (reference_count_ > 0) {
            lost_all_references_cond_.wait();
        }
        ready_to_delete_ = true;
    }
    ~snag_pointee_mixin_t() {
        guarantee(ready_to_delete_);
    }
private:
    void incr_reference_count() {
        ++ reference_count_;
    }

    void decr_reference_count() {
        -- reference_count_;
        if (reference_count_ == 0) {
            lost_all_references_cond_.pulse();
        }
    }


    int64_t reference_count_;
    cond_t lost_all_references_cond_;

    // Used by the destructor to ensure that the subclass has called
    // wait_until_ready_to_delete.
    bool ready_to_delete_;

    DISABLE_COPYING(snag_pointee_mixin_t);
};


// TODO: Make sure this implementation is really really correct.

// The type T must have a home_thread field.  It would probably be a
// subclass of home_thread_mixin_t.
template <class T>
class snag_ptr_t {
public:
    snag_ptr_t(const snag_ptr_t& other)
        : ptr_(other.ptr_) {
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::incr_reference_count, ptr_));
        }
    }

    explicit snag_ptr_t(T *ptr = NULL) : ptr_(ptr) {
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::incr_reference_count, ptr_));
        }
    }

    ~snag_ptr_t() {
        if (ptr_) {
            do_on_thread(ptr_->home_thread, boost::bind(&T::decr_reference_count, ptr_));
        }
    }

    void reset(T *ptr = NULL) {
        // The ordering of these operations is important, for the case
        // when ptr_ == ptr or some case where ptr_ holds a reference
        // to ptr.
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
