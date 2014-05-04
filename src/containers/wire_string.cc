// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/wire_string.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <limits>

#include "containers/archive/varint.hpp"
#include "containers/scoped.hpp"
#include "utils.hpp"

scoped_ptr_t<wire_string_t> wire_string_t::create(size_t size) {
    // This allocates size + 1 bytes for the data_ field (which is declared as char[1])
    size_t memory_size = sizeof(wire_string_t) + size;
    void *raw_result = ::rmalloc(memory_size);
    wire_string_t *result = static_cast<wire_string_t *>(raw_result);
    result->size_ = size;
    // Append a 0 character to allow for an efficient `c_str()`
    result->data_[size] = '\0';
    return scoped_ptr_t<wire_string_t>(result);
}
scoped_ptr_t<wire_string_t> wire_string_t::create_and_init(size_t size, const char *data) {
    scoped_ptr_t<wire_string_t> result = wire_string_t::create(size);
    memcpy(result->data_, data, size);
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
    if (size_ != other.size_) {
        return false;
    }

    return compare(other) == 0;
}
bool wire_string_t::operator!=(const wire_string_t &other) const {
    return !operator==(other);
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

scoped_ptr_t<wire_string_t> concat(const wire_string_t &a, const wire_string_t &b) {
    scoped_ptr_t<wire_string_t> result(wire_string_t::create(a.size() + b.size()));
    char *data = result->data();
    memcpy(data, a.data(), a.size());
    memcpy(data + a.size(), b.data(), b.size());
    return result;
}

size_t serialized_size(const wire_string_t &s) {
    return varint_uint64_serialized_size(s.size()) + s.size();
}

void serialize(write_message_t *wm, const wire_string_t &s) {
    serialize_varint_uint64(wm, static_cast<uint64_t>(s.size()));
    wm->append(s.data(), s.size());
}

archive_result_t deserialize(read_stream_t *s, scoped_ptr_t<wire_string_t> *out) {
    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res != archive_result_t::SUCCESS) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    scoped_ptr_t<wire_string_t> value(wire_string_t::create(sz));

    int64_t num_read = force_read(s, value->data(), sz);
    if (num_read == -1) {
        return archive_result_t::SOCK_ERROR;
    }
    if (static_cast<uint64_t>(num_read) < sz) {
        return archive_result_t::SOCK_EOF;
    }

    *out = std::move(value);

    return archive_result_t::SUCCESS;
}
