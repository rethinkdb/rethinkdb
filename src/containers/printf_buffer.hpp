// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_PRINTF_BUFFER_HPP_
#define CONTAINERS_PRINTF_BUFFER_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Cannot include utils.hpp, we are included by utils.hpp.
#include "errors.hpp"

// A static buffer for printing format strings, but it has a failsafe
// in case the buffer grows too large.
class printf_buffer_t {
public:
    printf_buffer_t();
    explicit printf_buffer_t(const char *format, ...) ATTR_FORMAT(printf, 2, 3);
    printf_buffer_t(va_list ap, const char *format) ATTR_FORMAT(printf, 3, 0);
    ~printf_buffer_t();

    // append and vappend become slow if the total size grows to N or
    // greater.  We still have std::vector-style amortized constant
    // time growth, though.
    void appendf(const char *format, ...) ATTR_FORMAT(printf, 2, 3);
    void vappendf(const char *format, va_list ap) ATTR_FORMAT(printf, 2, 0);

    char *data() const { return ptr_; }
    const char *c_str() const { return ptr_; }
    int64_t size() const { return length_; }

    static const int STATIC_DATA_SIZE = 1000;

private:
    // The number of bytes that have been appended.
    int64_t length_;

    // Either a pointer to data_, or, if length_ >= N, a pointer to an
    // array on the heap whose size is a power of two that is at least
    // length_ + 1.  Also, ptr_[length_] == 0, because we store a nul
    // terminator.
    char *ptr_;

    // A data buffer for avoiding allocation in the common case.
    char data_[STATIC_DATA_SIZE];


    DISABLE_COPYING(printf_buffer_t);
};



#endif  // CONTAINERS_PRINTF_BUFFER_HPP_
