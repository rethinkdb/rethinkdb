#ifndef CONTAINERS_REFC_PTR_HPP_
#define CONTAINERS_REFC_PTR_HPP_

#include "utils.hpp"

// Users of this class must implement two overloads:
//
//     void refc_ptr_add_ref(T *obj)
//
// Increments the reference count in obj, may not throw exceptions.
//
//     void refc_ptr_release(T *)
//
// Decrements the reference count in obj, and releases it if the
// reference count has hit zero.  May not throw exceptions.

template <class T>
class refc_ptr {
public:
    refc_ptr() : px_(NULL) { }

    // Takes a pointer allocated on the heap (with a ref count of
    // zero) and claims ownership over it (making its ref count 1).
    explicit refc_ptr(T *unowned) : px_(NULL) {
        reset(unowned);
    }

    ~refc_ptr() {
        reset();
    }

    void copy_from(const refc_ptr& other) {
        reset(other.px_);
    }

    void reset(T *other = NULL) {
        if (other) {
            refc_ptr_add_ref(other);
        }
        if (px_) {
            refc_ptr_release(px_);
        }
        px_ = other;
    }

    T& operator*() const {
        rassert(px_);
        return *px_;
    }

    T *operator->() const {
        rassert(px_);
        return px_;
    }

    T *get() const {
        return px_;
    }

    void swap(refc_ptr& other) {
        T *tmp = other.px_;
        other.px_ = px_;
        px_ = tmp;
    }

    typedef void (refc_ptr::*bool_type)() const;
    void dummy_member_function() const { }

    operator bool_type() const {
        return px_ ? &refc_ptr::dummy_member_function : 0;
    }

private:
    DISABLE_COPYING(refc_ptr);
    // TODO(sam): Temporarily to easy transition.
    // void operator=(const refc_ptr& other);
    T *px_;
};

template <class T>
void swap(refc_ptr<T>& a, refc_ptr<T>& b) {
    a.swap(b);
}

#endif  // CONTAINERS_REFC_PTR_HPP_
