#ifndef __SCOPED_MALLOC_HPP__
#define __SCOPED_MALLOC_HPP__

#include "errors.hpp"

// For dumb structs that get malloc/free for allocation.

template <class T>
class scoped_malloc {
public:
    scoped_malloc() : ptr_(NULL) { }
    scoped_malloc(size_t n) : ptr_(reinterpret_cast<T *>(malloc(n))) { }
    scoped_malloc(const char *beg, const char *end) {
        rassert(beg <= end);
        size_t n = end - beg;
        ptr_ = reinterpret_cast<T *>(malloc(n));
        memcpy(ptr_, beg, n);
    }
    ~scoped_malloc() { free(ptr_); ptr_ = NULL; }

    T *get() { return ptr_; }
    T *operator->() { return ptr_; }
    T& operator*() { return *ptr_; }

    void reset() {
        scoped_malloc tmp;
        swap(tmp);
    }

    void swap(scoped_malloc& other) {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    template <class U>
    friend class scoped_malloc;

    template <class U>
    void reinterpret_swap(scoped_malloc<U>& other) {
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
    scoped_malloc(const scoped_malloc&);
    void operator=(const scoped_malloc&);
};

#endif  // __SCOPED_MALLOC_HPP__
