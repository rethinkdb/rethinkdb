#ifndef CONTAINERS_SCOPED_MALLOC_HPP_
#define CONTAINERS_SCOPED_MALLOC_HPP_

#include <string.h>

#include "errors.hpp"

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

#endif  // CONTAINERS_SCOPED_MALLOC_HPP_
