#ifndef __SCOPED_MALLOC_HPP__
#define __SCOPED_MALLOC_HPP__

// For dumb structs that get malloc/free for allocation.

template <class T>
class scoped_malloc {
public:
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
};


#endif  // __SCOPED_MALLOC_HPP__
