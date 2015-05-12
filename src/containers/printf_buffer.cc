// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/printf_buffer.hpp"
#include "math.hpp"

printf_buffer_t::printf_buffer_t() : length_(0), ptr_(data_) {
    data_[0] = 0;
}

printf_buffer_t::printf_buffer_t(const char *format, ...) : length_(0), ptr_(data_) {
    data_[0] = 0;

    va_list ap;
    va_start(ap, format);

    vappendf(format, ap);

    va_end(ap);
}

printf_buffer_t::printf_buffer_t(va_list ap, const char *format) : length_(0), ptr_(data_) {
    data_[0] = 0;

    vappendf(format, ap);
}

printf_buffer_t::~printf_buffer_t() {
    if (ptr_ != data_) {
        delete[] ptr_;
    }
}

void printf_buffer_t::appendf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vappendf(format, ap);

    va_end(ap);
}

ATTRIBUTE_FORMAT(printf, 5, 0)
inline void alloc_copy_and_format(const char *buf, int64_t length, int append_size, int64_t alloc_limit, const char *fmt, va_list ap, char **buf_out) {
    char *new_ptr = new char[alloc_limit];
    memcpy(new_ptr, buf, length);

    DEBUG_VAR int size = vsnprintf(new_ptr + length, size_t(append_size) + 1, fmt, ap);
    rassert(size == append_size);

    *buf_out = new_ptr;
}

void printf_buffer_t::vappendf(const char *format, va_list ap) {
    va_list aq;
    va_copy(aq, ap);

    if (ptr_ == data_) {
        rassert(length_ < STATIC_DATA_SIZE);

        // the snprintfs return the number of characters they _would_
        // have written, not including the '\0'.
        int size = vsnprintf(ptr_ + length_, STATIC_DATA_SIZE - length_, format, ap);
        rassert(size >= 0, "vsnprintf failed, bad format string?");

        if (size < STATIC_DATA_SIZE - length_) {
            length_ += size;
        } else {
            char *new_ptr;
            alloc_copy_and_format(ptr_, length_, size,
                                  int64_round_up_to_power_of_two(length_ + size + 1),
                                  format, aq, &new_ptr);

            // TODO: Have valgrind mark data_ memory undefined.
            ptr_ = new_ptr;
            length_ += size;
        }

    } else {
        rassert(length_ >= STATIC_DATA_SIZE);

        char tmp[1];

        int size = vsnprintf(tmp, sizeof(tmp), format, ap);
        rassert(size >= 0, "vsnprintf failed, bad format string?");

        int64_t alloc_limit = int64_round_up_to_power_of_two(length_ + 1);
        if (length_ + size + 1 <= alloc_limit) {
            DEBUG_VAR int size2 = vsnprintf(ptr_ + length_, size + 1, format, aq);
            rassert(size == size2);
            length_ += size;
        } else {

            char *new_ptr;
            alloc_copy_and_format(ptr_, length_, size,
                                  int64_round_up_to_power_of_two(length_ + size + 1),
                                  format, aq, &new_ptr);

            delete[] ptr_;
            ptr_ = new_ptr;
            length_ += size;
        }
    }

    va_end(aq);
}
