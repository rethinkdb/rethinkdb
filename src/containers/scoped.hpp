// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SCOPED_HPP_
#define CONTAINERS_SCOPED_HPP_

#include <string.h>

#include <utility>

#include "config/args.hpp"
#include "errors.hpp"
#include "utils.hpp"

// Like boost::scoped_ptr only with release, init, no bool conversion, no boost headers!
template <class T>
class scoped_ptr_t {
public:
    template <class U>
    friend class scoped_ptr_t;

    scoped_ptr_t() : ptr_(nullptr) { }
    explicit scoped_ptr_t(T *p) : ptr_(p) { }

    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_ptr_t(scoped_ptr_t &&movee) noexcept : ptr_(movee.ptr_) {
        movee.ptr_ = nullptr;
    }
    template <class U>
    scoped_ptr_t(scoped_ptr_t<U> &&movee) noexcept : ptr_(movee.ptr_) {
        movee.ptr_ = nullptr;
    }

    ~scoped_ptr_t() {
        reset();
    }

    scoped_ptr_t &operator=(scoped_ptr_t &&movee) noexcept {
        scoped_ptr_t tmp(std::move(movee));
        swap(tmp);
        return *this;
    }

    template <class U>
    scoped_ptr_t &operator=(scoped_ptr_t<U> &&movee) noexcept {
        scoped_ptr_t tmp(std::move(movee));
        swap(tmp);
        return *this;
    }

    // These 'init' functions are largely obsolete, because move semantics are a
    // better thing to use.
    template <class U>
    void init(scoped_ptr_t<U> &&movee) {
        rassert(ptr_ == NULL);

        operator=(std::move(movee));
    }

    // includes a sanity-check for first-time use.
    void init(T *value) {
        rassert(ptr_ == NULL);

        // This is like reset with an assert.
        T *tmp = ptr_;
        ptr_ = value;
        delete tmp;
    }

    void reset() {
        T *tmp = ptr_;
        ptr_ = nullptr;
        delete tmp;
    }

    MUST_USE T *release() {
        T *tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

    void swap(scoped_ptr_t &other) noexcept {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    T &operator*() const {
        rassert(ptr_);
        return *ptr_;
    }

    T *get() const {
        rassert(ptr_);
        return ptr_;
    }

    T *get_or_null() const {
        return ptr_;
    }

    T *operator->() const {
        rassert(ptr_);
        return ptr_;
    }

    bool has() const {
        return ptr_ != nullptr;
    }

    explicit operator bool() const {
        return ptr_ != nullptr;
    }

private:
    T *ptr_;

    DISABLE_COPYING(scoped_ptr_t);
};

template <class T, class... Args>
scoped_ptr_t<T> make_scoped(Args&&... args) {
    return scoped_ptr_t<T>(new T(std::forward<Args>(args)...));
}

// Not really like boost::scoped_array.  A fascist array.
template <class T>
class scoped_array_t {
public:
    scoped_array_t() : ptr_(nullptr), size_(0) { }
    explicit scoped_array_t(size_t n) : ptr_(nullptr), size_(0) {
        init(n);
    }

    scoped_array_t(T *ptr, size_t _size) : ptr_(nullptr), size_(0) {
        init(ptr, _size);
    }

    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_array_t(scoped_array_t &&movee) noexcept
        : ptr_(movee.ptr_), size_(movee.size_) {
        movee.ptr_ = nullptr;
        movee.size_ = 0;
    }

    ~scoped_array_t() {
        reset();
    }

    scoped_array_t &operator=(scoped_array_t &&movee) noexcept {
        scoped_array_t tmp(std::move(movee));
        swap(tmp);
        return *this;
    }

    void init(size_t n) {
        rassert(ptr_ == NULL);
        ptr_ = new T[n];
        size_ = n;
    }

    // The opposite of release.
    void init(T *ptr, size_t _size) {
        rassert(ptr != NULL);
        rassert(ptr_ == NULL);

        ptr_ = ptr;
        size_ = _size;
    }

    void reset() {
        T *tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        delete[] tmp;
    }

    MUST_USE T *release(size_t *size_out) {
        *size_out = size_;
        T *tmp = ptr_;
        ptr_ = NULL;
        size_ = 0;
        return tmp;
    }

    void swap(scoped_array_t &other) noexcept {
        T *tmp = ptr_;
        size_t tmpsize = size_;
        ptr_ = other.ptr_;
        size_ = other.size_;
        other.ptr_ = tmp;
        other.size_ = tmpsize;
    }

    T &operator[](size_t i) const {
        rassert(ptr_);
        rassert(i < size_);
        return ptr_[i];
    }

    T *data() const {
        rassert(ptr_);
        return ptr_;
    }

    size_t size() const {
        rassert(ptr_);
        return size_;
    }

    bool has() const {
        return ptr_ != NULL;
    }

private:
    T *ptr_;
    size_t size_;

    DISABLE_COPYING(scoped_array_t);
};

// This class has move semantics in its copy constructor.
// It is meant to be used instead of C++14 generalized lambda capture,
// to capture a variable using move semantics,
// which GCC 4.6 doesn't support.
template<class T>
class copyable_unique_t {
public:
    explicit copyable_unique_t(T&& x)
        : it(std::move(x)) { }
    copyable_unique_t(const copyable_unique_t<T> &other)
        : it(std::move(other.it)) { }
    T release() const {
        T x(std::move(it));
        return x;
    }
private:
    mutable T it;
};

#ifdef __clang__
#define CAN_ALIAS_TEMPLATES 1
#elif __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
#define CAN_ALIAS_TEMPLATES 0
#else
#define CAN_ALIAS_TEMPLATES 1
#endif

/*
 * For pointers with custom allocators and deallocators
 */
template <class T, void*(*alloc)(size_t), void(*dealloc)(void*)>
class scoped_alloc_t {

    static_assert(std::is_pod<T>::value || std::is_same<T, void>::value,
                  "refusing to malloc non-POD, non-void type");

public:
    template <class U, void*(*alloc_)(size_t), void(*dealloc_)(void*)>
    friend class scoped_alloc_t;

    scoped_alloc_t() : ptr_(nullptr) { }
    explicit scoped_alloc_t(size_t n) : ptr_(static_cast<T *>(alloc(n))) { }
    scoped_alloc_t(const void *beg, size_t n) {
        ptr_ = static_cast<T *>(alloc(n));
        memcpy(ptr_, beg, n);
    }
    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_alloc_t(scoped_alloc_t &&movee) noexcept
        : ptr_(movee.ptr_) {
        movee.ptr_ = nullptr;
    }

    template <class U>
    scoped_alloc_t(scoped_alloc_t<U, alloc, dealloc> &&movee) noexcept
        : ptr_(movee.ptr_) {
        movee.ptr_ = nullptr;
    }

    template <class U>
    scoped_alloc_t(copyable_unique_t<U> other) noexcept : ptr_(nullptr) { // NOLINT
        scoped_alloc_t tmp(other.release());
        swap(tmp);
    }

    void operator=(scoped_alloc_t &&movee) noexcept {
        scoped_alloc_t tmp(std::move(movee));
        swap(tmp);
    }

    T *get() const { return ptr_; }
    T *operator->() const { return ptr_; }

    bool has() const {
        return ptr_ != nullptr;
    }

    void reset() {
        scoped_alloc_t tmp;
        swap(tmp);
    }

#if !CAN_ALIAS_TEMPLATES
protected:
#endif
    ~scoped_alloc_t() {
        dealloc(ptr_);
    }

private:
    friend class released_t;

    explicit scoped_alloc_t(void*) = delete;

    void swap(scoped_alloc_t &other) noexcept {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    T *ptr_;

    DISABLE_COPYING(scoped_alloc_t);
};

template <int alignment>
void *raw_malloc_aligned(size_t size) {
    return raw_malloc_aligned(size, alignment);
}

#if !CAN_ALIAS_TEMPLATES

// GCC 4.6 doesn't support template aliases

#define TEMPLATE_ALIAS(type_t, ...)                                     \
    struct type_t : public __VA_ARGS__ {                                \
        template <class ... arg_ts>                                     \
        type_t(arg_ts&&... args) : /* NOLINT */                         \
            __VA_ARGS__(std::forward<arg_ts>(args)...) { }              \
    }

#else

#define TEMPLATE_ALIAS(type_t, ...) using type_t = __VA_ARGS__;

#endif

// A type for pointers using rmalloc/free
template <class T>
TEMPLATE_ALIAS(scoped_malloc_t, scoped_alloc_t<T, rmalloc, free>);

// A type for aligned pointers
// Needed because, on Windows, raw_free_aligned doesn't call free
template <class T, int alignment>
TEMPLATE_ALIAS(scoped_aligned_ptr_t, scoped_alloc_t<T, raw_malloc_aligned<alignment>, raw_free_aligned>);

#ifndef _WIN32
// A type for page-aligned pointers
template <class T>
TEMPLATE_ALIAS(scoped_page_aligned_ptr_t, scoped_alloc_t<T, raw_malloc_page_aligned, raw_free_aligned>);
#endif

// A type for device-block-aligned pointers
template <class T>
TEMPLATE_ALIAS(scoped_device_block_aligned_ptr_t, scoped_alloc_t<T, raw_malloc_aligned<DEVICE_BLOCK_SIZE>, raw_free_aligned>);

#endif  // CONTAINERS_SCOPED_HPP_
