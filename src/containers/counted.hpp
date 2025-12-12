// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file counted.hpp
/// @brief Reference-counted smart pointer for shared object ownership
///
/// Provides a boost::intrusive_ptr-like smart pointer for reference-counted
/// objects. Objects are automatically deleted when the last reference is released.
/// Includes support for checking if an object is uniquely owned (.unique()).
///
/// @defgroup SmartPointers Reference-Counted Smart Pointers
/// Smart pointers for managing shared object ownership
/// @{

#ifndef CONTAINERS_COUNTED_HPP_
#define CONTAINERS_COUNTED_HPP_

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <utility>

#include "containers/scoped.hpp"
#include "errors.hpp"
#include "threading.hpp"

/// @brief Reference-counted smart pointer for shared ownership
///
/// `counted_t` provides automatic reference counting and memory management.
/// It's similar to boost::intrusive_ptr and manages objects that implement
/// reference counting via `counted_add_ref()` and `counted_release()` functions.
///
/// The managed type T must provide these functions:
/// - `void counted_add_ref(T *p)` - Increment reference count
/// - `void counted_release(T *p)` - Decrement reference count and potentially delete
///
/// @tparam T The type of object being reference-counted
/// @note Provides .unique() to check if this is the sole owner
/// @example
/// @code
/// class MyObject : public counted_t_base {
/// public:
///     void do_something() { /* ... */ }
/// };
///
/// {
///     counted_t<MyObject> ptr1(new MyObject());
///     counted_t<MyObject> ptr2 = ptr1;  // Both own the object
///     
///     ptr1->do_something();
///     if (!ptr2.unique()) {
///         // Shared ownership detected
///     }
/// }  // Object deleted here when last counted_t is destroyed
/// @endcode
///
/// @note This is a clone of boost::intrusive_ptr with added .unique() support
template <class T>
class counted_t {
public:
    template <class U>
    friend class counted_t;

    /// @brief Default constructor creates null pointer
    counted_t() : p_(nullptr) { }

    /// @brief Constructs from a raw pointer, taking ownership
    /// Calls counted_add_ref() on the pointer
    /// @param p Raw pointer to reference-counted object
    explicit counted_t(T *p) : p_(p) {
        if (p_ != nullptr) { counted_add_ref(p_); }
    }

    /// @brief Constructs from a scoped_ptr, taking ownership
    /// Extracts the pointer from scoped_ptr and calls counted_add_ref()
    /// @param p scoped_ptr to extract from
    explicit counted_t(scoped_ptr_t<T> &&p) : p_(p.release()) {
        if (p_ != nullptr) { counted_add_ref(p_); }
    }

    /// @brief Copy constructor increments reference count
    /// @param copyee The counted_t to copy from
    counted_t(const counted_t &copyee) : p_(copyee.p_) {
        if (p_ != nullptr) { counted_add_ref(p_); }
    }

    /// @brief Copy constructor with type conversion
    /// Allows implicit conversion from derived types
    /// @tparam U Type convertible to T*
    /// @param copyee The counted_t with different type
    template <class U>
    counted_t(const counted_t<U> &copyee) : p_(copyee.p_) {
        if (p_ != nullptr) { counted_add_ref(p_); }
    }

    /// @brief Move constructor transfers ownership
    /// Does not increment reference count (just moves pointer)
    /// @param movee The source counted_t (will be emptied)
    counted_t(counted_t &&movee) noexcept : p_(movee.p_) {
        movee.p_ = nullptr;
    }

    /// @brief Move constructor with type conversion
    /// @tparam U Type convertible to T*
    /// @param movee The source with different type
    template <class U>
    counted_t(counted_t<U> &&movee) noexcept : p_(movee.p_) {
        movee.p_ = nullptr;
    }

// Avoid a spurious warning when building on Saucy. See issue #1674
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 408)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

    ~counted_t() {
        if (p_) { counted_release(p_); }
    }

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 408)
#pragma GCC diagnostic pop
#endif

    void swap(counted_t &other) noexcept {
        T *tmp = p_;
        p_ = other.p_;
        other.p_ = tmp;
    }

    counted_t &operator=(counted_t &&other) noexcept {
        counted_t tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    template <class U>
    counted_t &operator=(counted_t<U> &&other) noexcept {
        counted_t tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    counted_t &operator=(const counted_t &other) {
        counted_t tmp(other);
        swap(tmp);
        return *this;
    }

    void reset() {
        counted_t tmp;
        swap(tmp);
    }

    void reset(T *other) {
        counted_t tmp(other);
        swap(tmp);
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
        return p_ != nullptr;
    }

    bool unique() const {
        return counted_use_count(p_) == 1;
    }

    explicit operator bool() const {
        return p_ != nullptr;
    }

private:
    T *p_;
};

template <class T, class... Args>
counted_t<T> make_counted(Args &&... args) {
    return counted_t<T>(new T(std::forward<Args>(args)...));
}

template <class> class single_threaded_countable_t;

template <class T>
inline void counted_add_ref(const single_threaded_countable_t<T> *p);
template <class T>
inline void counted_release(const single_threaded_countable_t<T> *p);
template <class T>
inline intptr_t counted_use_count(const single_threaded_countable_t<T> *p);

template <class T>
class single_threaded_countable_t : private home_thread_mixin_debug_only_t {
public:
    single_threaded_countable_t() : refcount_(0) { }

protected:
    counted_t<T> counted_from_this() {
        assert_thread();
        rassert(refcount_ > 0);
        return counted_t<T>(static_cast<T *>(this));
    }

    counted_t<const T> counted_from_this() const {
        assert_thread();
        rassert(refcount_ > 0);
        return counted_t<const T>(static_cast<const T *>(this));
    }

    ~single_threaded_countable_t() {
        assert_thread();
        rassert(refcount_ == 0);
    }

private:
    friend void counted_add_ref<T>(const single_threaded_countable_t<T> *p);
    friend void counted_release<T>(const single_threaded_countable_t<T> *p);
    friend intptr_t counted_use_count<T>(const single_threaded_countable_t<T> *p);

    mutable intptr_t refcount_;
    DISABLE_COPYING(single_threaded_countable_t);
};

template <class T>
inline void counted_add_ref(const single_threaded_countable_t<T> *p) {
    p->assert_thread();
    p->refcount_ += 1;
    rassert(p->refcount_ > 0);
}

template <class T>
inline void counted_release(const single_threaded_countable_t<T> *p) {
    p->assert_thread();
    rassert(p->refcount_ > 0);
    --p->refcount_;
    if (p->refcount_ == 0) {
        delete static_cast<T *>(const_cast<single_threaded_countable_t<T> *>(p));
    }
}

template <class T>
inline intptr_t counted_use_count(const single_threaded_countable_t<T> *p) {
    p->assert_thread();
    return p->refcount_;
}

template <class> class slow_atomic_countable_t;

template <class T>
inline void counted_add_ref(const slow_atomic_countable_t<T> *p);
template <class T>
inline void counted_release(const slow_atomic_countable_t<T> *p);
template <class T>
inline intptr_t counted_use_count(const slow_atomic_countable_t<T> *p);

template <class T>
class slow_atomic_countable_t {
public:
    slow_atomic_countable_t() : refcount_(0) { }

protected:
    ~slow_atomic_countable_t() {
        rassert(refcount_ == 0);
    }

    counted_t<T> counted_from_this() {
        rassert(counted_use_count(this) > 0);
        return counted_t<T>(static_cast<T *>(this));
    }

    counted_t<const T> counted_from_this() const {
        rassert(counted_use_count(this) > 0);
        return counted_t<const T>(static_cast<const T *>(this));
    }

private:
    friend void counted_add_ref<T>(const slow_atomic_countable_t<T> *p);
    friend void counted_release<T>(const slow_atomic_countable_t<T> *p);
    friend intptr_t counted_use_count<T>(const slow_atomic_countable_t<T> *p);

    mutable std::atomic<intptr_t> refcount_;
    DISABLE_COPYING(slow_atomic_countable_t);
};

template <class T>
inline void counted_add_ref(const slow_atomic_countable_t<T> *p) {
    DEBUG_VAR intptr_t res = ++(p->refcount_);
    rassert(res > 0);
}

template <class T>
inline void counted_release(const slow_atomic_countable_t<T> *p) {
    intptr_t res = --(p->refcount_);
    rassert(res >= 0);
    if (res == 0) {
        delete static_cast<T *>(const_cast<slow_atomic_countable_t<T> *>(p));
    }
}

template <class T>
inline intptr_t counted_use_count(const slow_atomic_countable_t<T> *p) {
    // Finally a practical use for volatile.
    intptr_t tmp = static_cast<const volatile intptr_t&>(p->refcount_);
    rassert(tmp > 0);
    return tmp;
}


// A noncopyable reference to a reference-counted object.
template <class T>
class movable_t {
public:
    explicit movable_t(const counted_t<T> &copyee) : ptr_(copyee) { }
    movable_t(movable_t &&movee) noexcept : ptr_(std::move(movee.ptr_)) { }
    movable_t &operator=(movable_t &&movee) noexcept {
        ptr_ = std::move(movee.ptr_);
        return *this;
    }

    void reset() { ptr_.reset(); }

    T *get() const { return ptr_.get(); }
    T *operator->() const { return ptr_.get(); }
    T &operator*() const { return *ptr_; }

    bool has() const { return ptr_.has(); }

private:
    counted_t<T> ptr_;
    DISABLE_COPYING(movable_t);
};

// Extends an arbitrary object with a slow_atomic_countable_t
template<class T>
class countable_wrapper_t : public T,
                            public slow_atomic_countable_t<countable_wrapper_t<T> > {
public:
    template <class... Args>
    explicit countable_wrapper_t(Args &&... args)
        : T(std::forward<Args>(args)...) { }
};

#endif  // CONTAINERS_COUNTED_HPP_
