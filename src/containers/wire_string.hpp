// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_WIRE_STRING_HPP_
#define CONTAINERS_WIRE_STRING_HPP_

#include <stdlib.h>
#include <string.h>
#include <string>

#include "utils.hpp"
#include "containers/archive/archive.hpp"

// TODO! Document
struct wire_string_t {
    wire_string_t() = delete;
    ~wire_string_t() = delete;

    static wire_string_t *create(size_t _size) {
        rassert(sizeof(_size) <= sizeof(uint64_t));
        size_t memory_size = sizeof(uint64_t) + _size*sizeof(char) + sizeof(char);
        void *raw_result = ::malloc(memory_size);
        // TODO! Check return value
        wire_string_t *result = reinterpret_cast<wire_string_t *>(raw_result);
        result->size_ = static_cast<uint64_t>(_size);
        // Append a 0 character to allow for an efficient `c_str()`
        result->data_[_size] = '\0';
        return result;
    }
    static wire_string_t *create_and_init(size_t _size, const char *_data) {
        wire_string_t *result = create(_size);
        memcpy(result->data_, _data, _size);
        return result;
    }
    static void destroy(wire_string_t *s) {
        ::free(s);
    }

    const char *c_str() const {
        return data_;
    }

    char *data() {
        return data_;
    }
    const char *data() const {
        return data_;
    }

    size_t length() const {
        return static_cast<size_t>(size_);
    }

    size_t memory_size() const {
        // TODO! Make this more robust. Probably use offset(data_)
        return sizeof(uint64_t) + size_*sizeof(char) + sizeof(char);
    }

    int compare(const wire_string_t &other) const {
        size_t common_size = static_cast<size_t>(std::min(size_, other.size_));
        int content_compare = memcmp(data_, other.data_, common_size);
        if (content_compare == 0) {
            // Both strings have the same first `content_compare` characters.
            // Compare their lengths.
            if (size_ < other.size_) {
                return -1;
            } else if (size_ > other.size_) {
                return 1;
            } else {
                return 0;
            }
        } else {
            return content_compare;
        }
    }

    bool operator==(const wire_string_t &other) const {
        return compare(other) == 0;
    }
    bool operator==(const char *other) const {
        rassert(data_[size_] == '\0');
        return strcmp(data_, other) == 0;
    }

    operator std::string() const {
        return std::string(data_, size_);
    }

    wire_string_t *operator+(const wire_string_t &other) const {
        wire_string_t *result = create(size_ + other.size_);
        memcpy(result->data_, data_, size_);
        memcpy(&result->data_[size_], other.data_, other.size_);
        return result;
    }

private:
    uint64_t size_; // TODO! Make this a size_t
    char data_[0];

    DISABLE_COPYING(wire_string_t);
} __attribute__((__packed__));


size_t serialized_size(const wire_string_t *s);

write_message_t &operator<<(write_message_t &msg, const wire_string_t *s);

archive_result_t deserialize(read_stream_t *s, wire_string_t **out);

#endif  // CONTAINERS_WIRE_STRING_HPP_
