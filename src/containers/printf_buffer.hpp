#ifndef CONTAINERS_PRINTF_BUFFER_HPP_
#define CONTAINERS_PRINTF_BUFFER_HPP_

#include <stdarg.h>
#include <stdio.h>

#include "errors.hpp"


template <int N>
class printf_buffer_t {
public:
    printf_buffer_t();
    printf_buffer_t(const char *format, ...) __attribute__((format (printf, 2, 3)));
    printf_buffer_t(va_list ap, const char *format);
    ~printf_buffer_t();

    void init(const char *format, ...);
    void vinit(const char *format, va_list ap);


    char *data() const {
        rassert(ptr_);
        return ptr_;
    }
    const char *c_str() const {
        rassert(ptr_);
        return ptr_;
    }
    int size() const {
        rassert(ptr_);
        return length_;
    }

private:

    int length_;
    char *ptr_;
    char data_[N];


    DISABLE_COPYING(printf_buffer_t);
};

template <int N>
printf_buffer_t<N>::printf_buffer_t() : length_(0), ptr_(0) {
    data_[0] = 0;
}

template <int N>
printf_buffer_t<N>::printf_buffer_t(const char *format, ...) : length_(0), ptr_(0) {
    data_[0] = 0;

    va_list ap;
    va_start(ap, format);

    vinit(format, ap);

    va_end(ap);
}

template <int N>
printf_buffer_t<N>::printf_buffer_t(va_list ap, const char *format) : length_(0), ptr_(0) {
    data_[0] = 0;
    vinit(format, ap);
}

template <int N>
printf_buffer_t<N>::~printf_buffer_t() {
    if (ptr_ != data_) {
        delete[] ptr_;
    }
}

template <int N>
void printf_buffer_t<N>::init(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vinit(format, ap);

    va_end(ap);
}

template <int N>
void printf_buffer_t<N>::vinit(const char *format, va_list ap) {
    rassert(!ptr_);

    va_list aq;
    va_copy(aq, ap);

    // the snprintfs return the number of characters they _would_
    // have written, not including the '\0'.
    int size = vsnprintf(data_, N, format, ap);

    rassert(size >= 0, "vsnprintf failed, bad format string?");

    if (size < N) {
        ptr_ = data_;
        length_ = size;
        return;
    }

    ptr_ = new char[size + 1];
    length_ = vsnprintf(ptr_, size + 1, format, aq);
    rassert(length_ == size);

    va_end(aq);
}




#endif  // CONTAINERS_PRINTF_BUFFER_HPP_
