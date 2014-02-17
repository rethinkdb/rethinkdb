// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/wire_string.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits>

#include "containers/archive/varint.hpp"
#include "utils.hpp"

wire_string_t *wire_string_t::create(size_t _size) {
    // This allocates _size + 1 bytes for the data_ field (which is declared as char[1])
    size_t memory_size = sizeof(wire_string_t) + _size;
    void *raw_result = ::malloc(memory_size);
    guarantee(raw_result != NULL, "Unable to allocate memory.");
    wire_string_t *result = reinterpret_cast<wire_string_t *>(raw_result);
    result->size_ = _size;
    // Append a 0 character to allow for an efficient `c_str()`
    result->data_[_size] = '\0';
    return result;
}
wire_string_t *wire_string_t::create_and_init(size_t _size, const char *_data) {
    wire_string_t *result = create(_size);
    memcpy(result->data_, _data, _size);
    return result;
}
void wire_string_t::operator delete(void *p) {
    ::free(p);
}

const char *wire_string_t::c_str() const {
    return data_;
}
char *wire_string_t::data() {
    return data_;
}
const char *wire_string_t::data() const {
    return data_;
}
size_t wire_string_t::size() const {
    return size_;
}

int wire_string_t::compare(const wire_string_t &other) const {
    size_t common_size = std::min(size_, other.size_);
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

bool wire_string_t::operator==(const char *other) const {
    rassert(data_[size_] == '\0');
    return strcmp(data_, other) == 0;
}
bool wire_string_t::operator==(const wire_string_t &other) const {
    return compare(other) == 0;
}
bool wire_string_t::operator!=(const wire_string_t &other) const {
    return compare(other) != 0;
}
bool wire_string_t::operator<(const wire_string_t &other) const {
    return compare(other) < 0;
}
bool wire_string_t::operator>(const wire_string_t &other) const {
    return compare(other) > 0;
}
bool wire_string_t::operator<=(const wire_string_t &other) const {
    return compare(other) <= 0;
}
bool wire_string_t::operator>=(const wire_string_t &other) const {
    return compare(other) >= 0;
}

std::string wire_string_t::to_std() const {
    return std::string(data_, size_);
}

wire_string_t *wire_string_t::operator+(const wire_string_t &other) const {
    wire_string_t *result = create(size_ + other.size_);
    memcpy(result->data_, data_, size_);
    memcpy(&result->data_[size_], other.data_, other.size_);
    return result;
}


size_t serialized_size(const wire_string_t &s) {
    return varint_uint64_serialized_size(s.size()) + s.size();
}

write_message_t &operator<<(write_message_t &msg, const wire_string_t &s) {
    serialize_varint_uint64(&msg, static_cast<uint64_t>(s.size()));
    msg.append(reinterpret_cast<const void *>(s.data()), s.size());
    return msg;
}

archive_result_t deserialize(read_stream_t *s, wire_string_t **out) THROWS_NOTHING {
    *out = NULL;

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res != ARCHIVE_SUCCESS) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return ARCHIVE_RANGE_ERROR;
    }

    *out = wire_string_t::create(sz);

    int64_t num_read = force_read(s, (*out)->data(), sz);
    if (num_read == -1) {
        delete *out;
        *out = NULL;
        return ARCHIVE_SOCK_ERROR;
    }
    if (static_cast<uint64_t>(num_read) < sz) {
        delete *out;
        *out = NULL;
        return ARCHIVE_SOCK_EOF;
    }

    return ARCHIVE_SUCCESS;
}
