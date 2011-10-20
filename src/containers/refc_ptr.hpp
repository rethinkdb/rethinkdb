#ifndef CONTAINERS_REFC_PTR_HPP_
#define CONTAINERS_REFC_PTR_HPP_

#include "utils.hpp"

// An intrusive_ptr-like pointer with no implicit copy constructor or
// assignment operator.
//
// Users of this class must implement two overloads:
//
//     void refc_ptr_t_add_ref(T *obj)
//
//         Increments the reference count in obj.  May not throw
//         exceptions.
//
//     void refc_ptr_t_release(T *)
//
//         Decrements the reference count in obj, and releases it if
//         the reference count has hit zero.  May not throw
//         exceptions.
template <class T>
class refc_ptr_t {
public:
    refc_ptr_t() : px_(NULL) { }

    // Takes a pointer allocated on the heap (with a ref count of
    // zero) and claims ownership over it (making its ref count 1).
    explicit refc_ptr_t(T *unowned) : px_(NULL) {
        reset(unowned);
    }

    ~refc_ptr_t() {
        reset();
    }

    void copy_from(const refc_ptr_t& other) {
        reset(other.px_);
    }

    void reset(T *other = NULL) {
        if (other) {
            refc_ptr_t_add_ref(other);
        }
        if (px_) {
            refc_ptr_t_release(px_);
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

    void swap(refc_ptr_t& other) {
        T *tmp = other.px_;
        other.px_ = px_;
        px_ = tmp;
    }

    // A dummy bool type that can't get implicitly converted to int.
    typedef void (refc_ptr_t::*bool_type)() const;
    void dummy_member_function() const { }

    operator bool_type() const {
        return px_ ? &refc_ptr_t::dummy_member_function : 0;
    }

private:
    DISABLE_COPYING(refc_ptr_t);
    T *px_;
};

template <class T>
void swap(refc_ptr_t<T>& a, refc_ptr_t<T>& b) {
    a.swap(b);
}

#endif  // CONTAINERS_REFC_PTR_HPP_
