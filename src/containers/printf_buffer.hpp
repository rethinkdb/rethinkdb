// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file printf_buffer.hpp
/// @brief Efficient buffered printf-style string building
///
/// Provides a string buffer class that accumulates formatted output without
/// the overhead of std::string construction for each append. Uses a static
/// internal buffer for small strings and promotes to heap allocation only
/// when necessary.
///
/// @defgroup StringFormatting String Formatting and Buffering
/// Efficient buffered string construction with printf-style formatting
/// @{

#ifndef CONTAINERS_PRINTF_BUFFER_HPP_
#define CONTAINERS_PRINTF_BUFFER_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Cannot include utils.hpp, we are included by utils.hpp.
#include "errors.hpp"

/// @brief Static buffer for printf-style string building
///
/// A buffer that accumulates formatted strings without the overhead of
/// std::string operations. Uses a static internal buffer (1000 bytes) for
/// small strings and automatically promotes to heap-allocated memory for
/// larger strings.
///
/// @example
/// @code
/// printf_buffer_t buf;
/// buf.appendf("Processing item %d of %d", i, total);
/// buf.appendf(" (%.2f%%)", percentage);
///
/// std::string result = buf.c_str();  // Get as C-string
/// int64_t len = buf.size();          // Get length
/// @endcode
class printf_buffer_t {
public:
    /// @brief Constructs an empty buffer
    printf_buffer_t();

    /// @brief Constructs a buffer initialized with formatted text
    /// @param format Printf-style format string
    /// @param ... Variable arguments for formatting
    explicit printf_buffer_t(const char *format, ...) ATTR_FORMAT(printf, 2, 3);

    /// @brief Constructs a buffer from a va_list
    /// @param ap Variable argument list with format parameters
    /// @param format Printf-style format string
    printf_buffer_t(va_list ap, const char *format) ATTR_FORMAT(printf, 3, 0);

    /// @brief Destructor cleans up heap-allocated memory if needed
    ~printf_buffer_t();

    /// @brief Appends formatted text to the buffer
    /// The append operation is amortized O(1) for length growth,
    /// but becomes slower once the total size reaches the threshold.
    /// @param format Printf-style format string
    /// @param ... Variable arguments for formatting
    /// @example
    /// @code
    /// buf.appendf("Value: %d", value);
    /// @endcode
    void appendf(const char *format, ...) ATTR_FORMAT(printf, 2, 3);

    /// @brief Appends formatted text from a va_list
    /// @param format Printf-style format string
    /// @param ap Variable argument list with format parameters
    void vappendf(const char *format, va_list ap) ATTR_FORMAT(printf, 2, 0);

    /// @brief Returns a pointer to the buffer data
    /// @return Pointer to the internal buffer (not null-terminated)
    char *data() const { return ptr_; }

    /// @brief Returns the buffer as a null-terminated C-string
    /// @return Pointer to null-terminated string
    /// @note The buffer is always kept null-terminated for safety
    const char *c_str() const { return ptr_; }

    /// @brief Returns the current size of the buffer in bytes
    /// @return Number of bytes currently in the buffer (excluding null terminator)
    int64_t size() const { return length_; }

    /// @brief Static buffer size for typical use without heap allocation
    static const int STATIC_DATA_SIZE = 1000;

private:
    /// @internal Number of bytes currently appended to the buffer
    int64_t length_;

    /// @internal Pointer to buffer data
    /// Points to data_ for small strings, or heap-allocated memory for larger ones
    char *ptr_;

    /// @internal Static buffer to avoid heap allocation for small strings
    char data_[STATIC_DATA_SIZE];

    DISABLE_COPYING(printf_buffer_t);
};

/// @}

#endif  // CONTAINERS_PRINTF_BUFFER_HPP_
