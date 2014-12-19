// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SCOPED_HPP_
#define CONTAINERS_SCOPED_HPP_

#include <string.h>

#include <utility>

#include "errors.hpp"
#include "utils.hpp"

// Like boost::scoped_ptr only with release, init, no bool conversion, no boost headers!
template <class T>
class scoped_ptr_t {
public:
    template <class U>
    friend class scoped_ptr_t;

    scoped_ptr_t() : ptr_(NULL) { }
    explicit scoped_ptr_t(T *p) : ptr_(p) { }

    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_ptr_t(scoped_ptr_t &&movee) noexcept : ptr_(movee.ptr_) {
        movee.ptr_ = NULL;
    }
    template <class U>
    scoped_ptr_t(scoped_ptr_t<U> &&movee) noexcept : ptr_(movee.ptr_) {
        movee.ptr_ = NULL;
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
        ptr_ = NULL;
        delete tmp;
    }

    MUST_USE T *release() {
        T *tmp = ptr_;
        ptr_ = NULL;
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
        return ptr_ != NULL;
    }

    explicit operator bool() const {
        return ptr_ != NULL;
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
    scoped_array_t() : ptr_(NULL), size_(0) { }
    explicit scoped_array_t(size_t n) : ptr_(NULL), size_(0) {
        init(n);
    }

    scoped_array_t(T *ptr, size_t _size) : ptr_(NULL), size_(0) {
        init(ptr, _size);
    }

    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_array_t(scoped_array_t &&movee) noexcept
        : ptr_(movee.ptr_), size_(movee.size_) {
        movee.ptr_ = NULL;
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
        ptr_ = NULL;
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

// For dumb structs that get rmalloc/free for allocation.

template <class T>
class scoped_malloc_t {
public:
    template <class U>
    friend class scoped_malloc_t;

    scoped_malloc_t() : ptr_(NULL) { }
    explicit scoped_malloc_t(void *ptr) : ptr_(static_cast<T *>(ptr)) { }
    explicit scoped_malloc_t(size_t n) : ptr_(static_cast<T *>(rmalloc(n))) { }
    scoped_malloc_t(const char *beg, const char *end) {
        rassert(beg <= end);
        size_t n = end - beg;
        ptr_ = static_cast<T *>(rmalloc(n));
        memcpy(ptr_, beg, n);
    }
    // (These noexcepts don't actually do anything w.r.t. STL containers, since the
    // type's not copyable.  There is no specific reason why these are many other
    // functions need be marked noexcept with any degree of urgency.)
    scoped_malloc_t(scoped_malloc_t &&movee) noexcept
        : ptr_(movee.ptr_) {
        movee.ptr_ = NULL;
    }

    template <class U>
    scoped_malloc_t(scoped_malloc_t<U> &&movee) noexcept
        : ptr_(movee.ptr_) {
        movee.ptr_ = NULL;
    }

    ~scoped_malloc_t() {
        free(ptr_);
    }

    void operator=(scoped_malloc_t &&movee) noexcept {
        scoped_malloc_t tmp(std::move(movee));
        swap(tmp);
    }

    void init(void *ptr) {
        guarantee(ptr_ == NULL);
        ptr_ = static_cast<T *>(ptr);
    }

    T *get() const { return ptr_; }
    T *operator->() const { return ptr_; }

    T *release() {
        T *tmp = ptr_;
        ptr_ = NULL;
        return tmp;
    }

    void reset() {
        scoped_malloc_t tmp;
        swap(tmp);
    }

    bool has() const {
        return ptr_ != NULL;
    }

private:
    void swap(scoped_malloc_t &other) noexcept {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    T *ptr_;

    DISABLE_COPYING(scoped_malloc_t);
};

#endif  // CONTAINERS_SCOPED_HPP_
