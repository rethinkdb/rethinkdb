// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_WIRE_STRING_HPP_
#define CONTAINERS_WIRE_STRING_HPP_

#include <string>

#include "containers/archive/archive.hpp"
#include "containers/shared_buffer.hpp"

/* `wire_string_t` is a length-prefixed ("Pascal style") non-mutable string.
 * This has two advantages over C-strings:
 * - it can be efficiently serialized and deserialized
 * - it can contain any character, including '\0'
 *
 * Underneath `wire_string_t` uses a `shared_buf_ref_t`. This makes it
 * relatively cheap to copy.
 */
class wire_string_t {
public:
    // Creates an empty wire_string_t
    wire_string_t();

    // Creates a wire_string_t with its content copied from _data
    wire_string_t(size_t _size, const char *_data);

    // Construct from a C string
    explicit wire_string_t(const char *c_str);

    // Construct from an STD string
    explicit wire_string_t(const std::string &str);

    // Create a wire_string_t from an existing shared_buf_ref_t
    wire_string_t(size_t _size, const shared_buf_ref_t &_ref);
    wire_string_t(size_t _size, shared_buf_ref_t &&_ref);

    wire_string_t(wire_string_t &&movee) noexcept;
    wire_string_t(const wire_string_t &copyee) noexcept;

    wire_string_t &operator=(const wire_string_t &copyee) noexcept;
    wire_string_t &operator=(wire_string_t &&movee) noexcept;

    // The result of data() is not automatically null terminated. Do not use
    // as a C string.
    const char *data() const;
    size_t size() const;
    bool empty() const;

    int compare(const wire_string_t &other) const;

    // Short cut for comparing to C-strings and STD strings
    bool operator==(const char *other) const;
    bool operator!=(const char *other) const;
    bool operator==(const wire_string_t &other) const;
    bool operator!=(const wire_string_t &other) const;
    bool operator<(const wire_string_t &other) const;
    bool operator>(const wire_string_t &other) const;
    bool operator<=(const wire_string_t &other) const;
    bool operator>=(const wire_string_t &other) const;

    std::string to_std() const;

private:
    size_t size_;
    shared_buf_ref_t data_;
};


wire_string_t concat(const wire_string_t &a, const wire_string_t &b);

#endif  // CONTAINERS_WIRE_STRING_HPP_
