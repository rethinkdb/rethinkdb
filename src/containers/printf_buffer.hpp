#ifndef CONTAINERS_PRINTF_BUFFER_HPP_
#define CONTAINERS_PRINTF_BUFFER_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Cannot include utils.hpp, we are included by utils.hpp.
#include "errors.hpp"

int64_t round_up_to_power_of_two(int64_t x);



// A base class for printf_buffer_t, so that things which _use_ a
// printf buffer don't need to be templatized or know its size.
class append_only_printf_buffer_t {
public:
    virtual void appendf(const char *format, ...) = 0;
    virtual void vappendf(const char *format, va_list ap) = 0;
protected:
    append_only_printf_buffer_t() { }
    virtual ~append_only_printf_buffer_t() { }
private:
    DISABLE_COPYING(append_only_printf_buffer_t);
};

// A static buffer for printing format strings, but it has a failsafe
// in case the buffer grows too large.
template <int N>
class printf_buffer_t : public append_only_printf_buffer_t {
public:
    printf_buffer_t();
    explicit printf_buffer_t(const char *format, ...) __attribute__((format (printf, 2, 3)));
    printf_buffer_t(va_list ap, const char *format);
    ~printf_buffer_t();

    // append and vappend become slow if the total size grows to N or
    // greater.  We still have std::vector-style amortized constant
    // time growth, though.
    void appendf(const char *format, ...);
    void vappendf(const char *format, va_list ap);

    char *data() const {
        return ptr_;
    }
    const char *c_str() const {
        return ptr_;
    }
    int size() const {
        return length_;
    }

public:
    // The number of bytes that have been appended.
    int64_t length_;

    // Either a pointer to data_, or, if length_ >= N, a pointer to an
    // array on the heap whose size is a power of two that is at least
    // length_ + 1.  Also, ptr_[length_] == 0, because we store a nul
    // terminator.
    char *ptr_;

    // A data buffer for avoiding allocation in the common case.
    char data_[N];


    DISABLE_COPYING(printf_buffer_t);
};

template <int N>
printf_buffer_t<N>::printf_buffer_t() : length_(0), ptr_(data_) {
    data_[0] = 0;
}

template <int N>
printf_buffer_t<N>::printf_buffer_t(const char *format, ...) : length_(0), ptr_(data_) {
    data_[0] = 0;

    va_list ap;
    va_start(ap, format);

    vappendf(format, ap);

    va_end(ap);
}

template <int N>
printf_buffer_t<N>::printf_buffer_t(va_list ap, const char *format) : length_(0), ptr_(data_) {
    data_[0] = 0;

    vappendf(format, ap);
}

template <int N>
printf_buffer_t<N>::~printf_buffer_t() {
    if (ptr_ != data_) {
        delete[] ptr_;
    }
}

template <int N>
void printf_buffer_t<N>::appendf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vappendf(format, ap);

    va_end(ap);
}

inline void alloc_copy_and_format(const char *buf, int64_t length, int append_size, int64_t alloc_limit, const char *fmt, va_list ap, char **buf_out) {
    char *new_ptr = new char[alloc_limit];
    memcpy(new_ptr, buf, length);

    DEBUG_ONLY_VAR int size = vsnprintf(new_ptr + length, size_t(append_size) + 1, fmt, ap);
    rassert(size == append_size);

    *buf_out = new_ptr;
}

template <int N>
void printf_buffer_t<N>::vappendf(const char *format, va_list ap) {
    va_list aq;
    va_copy(aq, ap);

    if (ptr_ == data_) {
        rassert(length_ < N);

        // the snprintfs return the number of characters they _would_
        // have written, not including the '\0'.
        int size = vsnprintf(ptr_ + length_, N - length_, format, ap);
        rassertf(size >= 0, "vsnprintf failed, bad format string?");

        if (size < N - length_) {
            length_ += size;
        } else {
            char *new_ptr;
            alloc_copy_and_format(ptr_, length_, size, round_up_to_power_of_two(length_ + size + 1), format, aq, &new_ptr);

            // TODO: Have valgrind mark data_ memory undefined.
            ptr_ = new_ptr;
            length_ += size;
        }

    } else {
        rassert(length_ >= N);

        char tmp[1];

        int size = vsnprintf(tmp, sizeof(tmp), format, ap);
        rassertf(size >= 0, "vsnprintf failed, bad format string?");

        int64_t alloc_limit = round_up_to_power_of_two(length_ + 1);
        if (length_ + size + 1 <= alloc_limit) {
            DEBUG_ONLY_VAR int size2 = vsnprintf(ptr_ + length_, size + 1, format, aq);
            rassert(size == size2);
            length_ += size;
        } else {

            char *new_ptr;
            alloc_copy_and_format(ptr_, length_, size, round_up_to_power_of_two(length_ + size + 1), format, aq, &new_ptr);

            delete[] ptr_;
            ptr_ = new_ptr;
            length_ += size;
        }
    }

    va_end(aq);
}



#endif  // CONTAINERS_PRINTF_BUFFER_HPP_
