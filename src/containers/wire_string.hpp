// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_WIRE_STRING_HPP_
#define CONTAINERS_WIRE_STRING_HPP_

#include <string>

#include "containers/archive/archive.hpp"

/* `wire_string_t` is a length-prefixed ("Pascal style") string.
 * This has two advantages over C-strings:
 * - it can be efficiently serialized and deserialized
 * - it can contain any character, including '\0'
 * 
 * Its length cannot be changed once created.
 *
 * `wire_string_t` has no public constructors, and doesn't have
 * a fixed size. You can allocate one through the create() or create_and_init()
 * function. The returned object can be freed by using the delete operator.
 */
class wire_string_t {
public:
    wire_string_t() = delete;

    static wire_string_t *create(size_t _size);
    static wire_string_t *create_and_init(size_t _size, const char *_data);
    static void operator delete(void *p);

    // The memory pointed to by the result to c_str() is guaranteed to be null
    // terminated.
    const char *c_str() const;
    // .. the one returned by data() is not (though in the current implementation
    // it is null terminated as well). data() should be used in combination with
    // size().
    char *data();
    const char *data() const;
    size_t size() const;

    int compare(const wire_string_t &other) const;

    // Short cut for comparing to C-strings
    bool operator==(const char *other) const;
    bool operator==(const wire_string_t &other) const;
    bool operator!=(const wire_string_t &other) const;
    bool operator<(const wire_string_t &other) const;
    bool operator>(const wire_string_t &other) const;
    bool operator<=(const wire_string_t &other) const;
    bool operator>=(const wire_string_t &other) const;

    std::string to_std() const;

    // Creates a new wire_string_t and returns a pointer to it.
    // The new string is the concatenation of this and other.
    wire_string_t *operator+(const wire_string_t &other) const;

private:
    size_t size_;
    char data_[1];

    DISABLE_COPYING(wire_string_t);
};


size_t serialized_size(const wire_string_t &s);

write_message_t &operator<<(write_message_t &msg, const wire_string_t &s);

archive_result_t deserialize(read_stream_t *s, wire_string_t **out) THROWS_NOTHING;

#endif  // CONTAINERS_WIRE_STRING_HPP_
