#ifndef CONTAINERS_SCOPED_HPP_
#define CONTAINERS_SCOPED_HPP_

#include <string.h>

#include "errors.hpp"

// Like boost::scoped_ptr only with release, init, no bool conversion, no boost headers!
template <class T>
class scoped_ptr_t {
public:
    scoped_ptr_t() : ptr_(NULL) { }
    explicit scoped_ptr_t(T *p) : ptr_(p) { }
    ~scoped_ptr_t() {
        reset();
    }

    // reset with a sanity-check for first-time use.
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

    void swap(scoped_ptr_t& other) {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }



    T *get() const {
        rassert(ptr_);
        return ptr_;
    }

    T *operator->() const {
        rassert(ptr_);
        return ptr_;
    }

    bool has() const {
        return ptr_ != NULL;
    }


private:
    T *ptr_;

    DISABLE_COPYING(scoped_ptr_t);
};

// Not really like boost::scoped_array.  A fascist array.
template <class T>
class scoped_array_t {
public:
    scoped_array_t() : ptr_(NULL), size_(0) { }
    explicit scoped_array_t(ssize_t n) : ptr_(NULL), size_(0) {
        init(n);
    }

    scoped_array_t(T *ptr, ssize_t size) : ptr_(NULL), size_(0) {
        init(ptr, size);
    }

    ~scoped_array_t() {
        reset();
    }

    void init(ssize_t n) {
        rassert(ptr_ == NULL);
        rassert(n >= 0);

        ptr_ = new T[n];
        size_ = n;
    }

    // The opposite of release.
    void init(T *ptr, ssize_t size) {
        rassert(ptr != NULL);
        rassert(ptr_ == NULL);
        rassert(size >= 0);

        ptr_ = ptr;
        size_ = size;
    }

    void reset() {
        T *tmp = ptr_;
        ptr_ = NULL;
        size_ = 0;
        delete[] tmp;
    }

    MUST_USE T *release(ssize_t *size_out) {
        *size_out = size_;
        T *tmp = ptr_;
        ptr_ = NULL;
        size_ = 0;
        return tmp;
    }

    void swap(scoped_array_t &other) {
        T *tmp = ptr_;
        ssize_t tmpsize = size_;
        ptr_ = other.ptr_;
        size_ = other.size_;
        other.ptr_ = tmp;
        other.size_ = tmpsize;
    }



    T& operator[](ssize_t i) const {
        rassert(ptr_);
        rassert(0 <= i && i < size_);
        return ptr_[i];
    }

    T *data() const {
        rassert(ptr_);
        return ptr_;
    }

    ssize_t size() const {
        rassert(ptr_);
        return size_;
    }

    bool has() const {
        return ptr_ != NULL;
    }

private:
    T *ptr_;
    ssize_t size_;

    DISABLE_COPYING(scoped_array_t);
};


// For dumb structs that get malloc/free for allocation.

template <class T>
class scoped_malloc_t {
public:
    scoped_malloc_t() : ptr_(NULL) { }
    explicit scoped_malloc_t(size_t n) : ptr_(reinterpret_cast<T *>(malloc(n))) { }
    scoped_malloc_t(const char *beg, const char *end) {
        rassert(beg <= end);
        size_t n = end - beg;
        ptr_ = reinterpret_cast<T *>(malloc(n));
        memcpy(ptr_, beg, n);
    }
    ~scoped_malloc_t() {
        free(ptr_);
    }

    T *get() { return ptr_; }
    const T *get() const { return ptr_; }
    T *operator->() { return ptr_; }
    const T *operator->() const { return ptr_; }
    T& operator*() { return *ptr_; }

    void reset() {
        scoped_malloc_t tmp;
        swap(tmp);
    }

    void swap(scoped_malloc_t& other) {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    template <class U>
    friend class scoped_malloc_t;

    template <class U>
    void reinterpret_swap(scoped_malloc_t<U>& other) {
        T *tmp = ptr_;
        ptr_ = reinterpret_cast<T *>(other.ptr_);
        other.ptr_ = reinterpret_cast<U *>(tmp);
    }

    operator bool() const {
        return ptr_ != NULL;
    }

private:
    T *ptr_;

    // DISABLE_COPYING
    scoped_malloc_t(const scoped_malloc_t&);
    void operator=(const scoped_malloc_t&);
};

#endif  // CONTAINERS_SCOPED_HPP_
