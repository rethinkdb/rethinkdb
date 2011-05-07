#ifndef __SCOPED_MALLOC_HPP__
#define __SCOPED_MALLOC_HPP__

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

    void swap(scoped_malloc& other) {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

private:
    T *ptr_;

    // DISABLE_COPYING
    scoped_malloc(const scoped_malloc&);
    void operator=(const scoped_malloc&);
};

// The unique_ptr_t version of scoped_malloc.
template <class T>
class unique_malloc_t {
public:
    unique_malloc_t() : ptr_(NULL) { }
    unique_malloc_t(size_t n) : ptr_(reinterpret_cast<T *>(malloc(n))) { }
    unique_malloc_t(const char *beg, const char *end) {
        rassert(beg <= end);
        size_t n = end - beg;
        ptr_ = reinterpret_cast<T *>(malloc(n));
        memcpy(ptr_, beg, n);
    }
    unique_malloc_t(const unique_malloc_t& other) : ptr_(other.ptr_) {
        other.ptr_ = NULL;
    }
    ~unique_malloc_t() { free(ptr_); ptr_ = NULL; }

    unique_malloc_t& operator=(const unique_malloc_t& other) {
        free(ptr_);
        ptr_ = other.ptr_;
        other.ptr_ = NULL;
        return *this;
    }

    T *get() {
        rassert(ptr_ != NULL);
        return ptr_;
    }
    T *operator->() {
        rassert(ptr_ != NULL);
        return ptr_;
    }

private:
    mutable T *ptr_;
};

#endif  // __SCOPED_MALLOC_HPP__
