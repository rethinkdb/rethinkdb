#ifndef CONTAINERS_INTRUSIVE_PTR_HPP_
#define CONTAINERS_INTRUSIVE_PTR_HPP_

#include <stddef.h>
#include <stdint.h>

#include "errors.hpp"

// Yes, this is a clone of boost::intrusive_ptr.  This will probably
// not be the case in the future.

// Now it supports .unique(), and in order to use it, your type needs
// to provide an intrusive_ptr_use_count implementation.

template <class T>
class intrusive_ptr_t {
public:
    intrusive_ptr_t() : p_(NULL) { }
    explicit intrusive_ptr_t(T *p) : p_(p) {
        if (p_) { intrusive_ptr_add_ref(p_); }
    }

    intrusive_ptr_t(const intrusive_ptr_t &copyee) : p_(copyee.p_) {
        if (p_) { intrusive_ptr_add_ref(p_); }
    }

    ~intrusive_ptr_t() {
        if (p_) { intrusive_ptr_release(p_); }
    }

    void swap(intrusive_ptr_t &other) {
        T *tmp = p_;
        p_ = other.p_;
        other.p_ = tmp;
    }

    intrusive_ptr_t &operator=(const intrusive_ptr_t &other) {
        intrusive_ptr_t tmp(other);
        swap(tmp);
        return *this;
    }

    void reset() {
        intrusive_ptr_t tmp;
        swap(tmp);
    }

    void reset(T *other) {
        intrusive_ptr_t tmp(other);
        swap(tmp);  // NOLINT
    }

    T *operator->() const {
        return p_;
    }

    T &operator*() const {
        return *p_;
    }

    T *get() const {
        return p_;
    }

    bool has() const {
        return p_ != NULL;
    }

    bool unique() const {
        return intrusive_ptr_use_count(p_) == 1;
    }

    class hidden_t {
        hidden_t();
    };
    typedef void booleanesque_t(hidden_t);

    operator booleanesque_t*() const {
        return p_ ? &intrusive_ptr_t<T>::dummy_method : 0;
    }

private:
    static void dummy_method(hidden_t) { }

    T *p_;
};

template <class> class slow_shared_mixin_t;

template <class T>
inline void intrusive_ptr_add_ref(slow_shared_mixin_t<T> *p);
template <class T>
inline void intrusive_ptr_release(slow_shared_mixin_t<T> *p);
template <class T>
inline intptr_t intrusive_ptr_use_count(const slow_shared_mixin_t<T> *p);

template <class T>
class slow_shared_mixin_t {
public:
    slow_shared_mixin_t() : refcount_(0) { }

protected:
    ~slow_shared_mixin_t() { }

private:
    friend void intrusive_ptr_add_ref<T>(slow_shared_mixin_t<T> *p);
    friend void intrusive_ptr_release<T>(slow_shared_mixin_t<T> *p);
    friend intptr_t intrusive_ptr_use_count<T>(const slow_shared_mixin_t<T> *p);

    intptr_t refcount_;
    DISABLE_COPYING(slow_shared_mixin_t);
};

template <class T>
inline void intrusive_ptr_add_ref(slow_shared_mixin_t<T> *p) {
    DEBUG_VAR intptr_t res = __sync_add_and_fetch(&p->refcount_, 1);
    rassert(res > 0);
}

template <class T>
inline void intrusive_ptr_release(slow_shared_mixin_t<T> *p) {
    intptr_t res = __sync_sub_and_fetch(&p->refcount_, 1);
    rassert(res >= 0);
    if (res == 0) {
        delete static_cast<T *>(p);
    }
}

template <class T>
inline intptr_t intrusive_ptr_use_count(const slow_shared_mixin_t<T> *p) {
    // Finally a practical use for volatile.
    intptr_t tmp = static_cast<const volatile intptr_t&>(p->refcount_);
    rassert(tmp > 0);
    return tmp;
}


#endif  // CONTAINERS_INTRUSIVE_PTR_HPP_
