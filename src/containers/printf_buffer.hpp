#ifndef CONTAINERS_PRINTF_BUFFER_HPP_
#define CONTAINERS_PRINTF_BUFFER_HPP_

#include <stdarg.h>
#include <stdio.h>

#include "errors.hpp"

// A base class for printf_buffer_t, so that things which _use_ a
// printf buffer don't need to be templatized or know its size.
class append_only_printf_buffer_t {
public:
    virtual void append(const char *format, ...) = 0;
    virtual void vappend(const char *format, va_list ap) = 0;
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

    // append and vappend become slow if the total size grows to N or greater.
    void append(const char *format, ...);
    void vappend(const char *format, va_list ap);

    char *data() const {
        return ptr_;
    }
    const char *c_str() const {
        return ptr_;
    }
    int size() const {
        return length_;
    }

private:

    int length_;
    char *ptr_;
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

    vappend(format, ap);

    va_end(ap);
}

template <int N>
printf_buffer_t<N>::printf_buffer_t(va_list ap, const char *format) : length_(0), ptr_(data_) {
    data_[0] = 0;

    vappend(format, ap);
}

template <int N>
printf_buffer_t<N>::~printf_buffer_t() {
    if (ptr_ != data_) {
        delete[] ptr_;
    }
}

template <int N>
void printf_buffer_t<N>::append(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vappend(format, ap);

    va_end(ap);
}

template <int N>
void printf_buffer_t<N>::vappend(const char *format, va_list ap) {
    va_list aq;
    va_copy(aq, ap);

    if (ptr_ == data_) {
        rassert(length_ < N);

        // the snprintfs return the number of characters they _would_
        // have written, not including the '\0'.
        int size = vsnprintf(data_ + length_, N - length_, format, ap);
        rassert(size >= 0, "vsnprintf failed, bad format string?");

        if (size < N - length_) {
            length_ += size;
        } else {
            char *new_ptr = new char[length_ + size + 1];
            memcpy(new_ptr, data_, length_);

            // TODO: Use valgrind to mark data_ memory undefined.
            data_[0] = '\0';

            int size2 = vsnprintf(ptr_ + length_, size + 1, format, aq);
            rassert(size == size2);

            ptr_ = new_ptr;
            length_ += size;
        }

    } else {
        rassert(length_ >= N);

        char tmp[1];

        int size = vsnprintf(tmp, 1, format, ap);
        rassert(size >= 0, "vsnprintf failed, bad format string?");

        char *new_ptr = new char[length_ + size + 1];
        memcpy(new_ptr, ptr_, length_);

        int size2 = vsnprintf(new_ptr + length_, size + 1, format, aq);
        rassert(size == size2);

        ptr_ = new_ptr;
        length_ += size;
    }

    va_end(aq);
}



#endif  // CONTAINERS_PRINTF_BUFFER_HPP_
