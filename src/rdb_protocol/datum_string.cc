// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/datum_string.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <limits>

#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_stream.hpp"
#include "containers/archive/varint.hpp"
#include "containers/scoped.hpp"
#include "debug.hpp"
#include "utils.hpp"

datum_string_t::datum_string_t() {
    init(0, "");
}

datum_string_t::datum_string_t(size_t _size, const char *_data) {
    init(_size, _data);
}

datum_string_t::datum_string_t(const shared_buf_ref_t<char> &_ref)
    : data_(_ref) { }

datum_string_t::datum_string_t(shared_buf_ref_t<char> &&_ref)
    : data_(std::move(_ref)) { }

datum_string_t::datum_string_t(const char *c_str) {
    init(strlen(c_str), c_str);
}

datum_string_t::datum_string_t(const std::string &str) {
    init(str.size(), str.data());
}

void datum_string_t::init(size_t _size, const char *_data) {
    const size_t str_offset = varint_uint64_serialized_size(_size);
    counted_t<shared_buf_t> data = shared_buf_t::create(str_offset + _size);
    serialize_varint_uint64_into_buf(_size, reinterpret_cast<uint8_t *>(data->data()));
    memcpy(data->data() + str_offset, _data, _size);
    data_ = shared_buf_ref_t<char>(std::move(data), 0);
}

const char *datum_string_t::data() const {
    const size_t str_size = size();
    size_t data_offset = varint_uint64_serialized_size(str_size);
    data_.guarantee_in_boundary(data_offset + str_size);
    return data_.get() + data_offset;
}

size_t datum_string_t::size() const {
    uint64_t res = 0;
    static_assert(sizeof(uint8_t) == sizeof(char), "sizeof(uint8_t) != sizeof(char)");
    buffer_read_stream_t data_stream(data_.get(), data_.get_safety_boundary());
    guarantee_deserialization(deserialize_varint_uint64(&data_stream, &res),
                              "wire_string size");
    guarantee(res <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()));
    return static_cast<size_t>(res);
}

bool datum_string_t::empty() const {
    return size() == 0;
}

int datum_string_t::compare(const datum_string_t &other) const {
    return compare(other.size(), other.data());
}

int datum_string_t::compare(size_t other_size, const char *other_data) const {
    const size_t this_size = size();
    size_t common_size = std::min(this_size, other_size);
    int content_compare = memcmp(data(), other_data, common_size);
    if (content_compare == 0) {
        // Both strings have the same first `content_compare` characters.
        // Compare their lengths.
        if (this_size < other_size) {
            return -1;
        } else if (this_size > other_size) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return content_compare;
    }
}

bool datum_string_t::operator==(const char *other) const {
    return compare(strlen(other), other) == 0;
}
bool datum_string_t::operator!=(const char *other) const {
    return !(*this == other);
}

bool datum_string_t::operator==(const datum_string_t &other) const {
    if (size() != other.size()) {
        return false;
    }

    return compare(other) == 0;
}
bool datum_string_t::operator!=(const datum_string_t &other) const {
    return !operator==(other);
}
bool datum_string_t::operator<(const datum_string_t &other) const {
    return compare(other) < 0;
}
bool datum_string_t::operator>(const datum_string_t &other) const {
    return compare(other) > 0;
}
bool datum_string_t::operator<=(const datum_string_t &other) const {
    return compare(other) <= 0;
}
bool datum_string_t::operator>=(const datum_string_t &other) const {
    return compare(other) >= 0;
}

std::string datum_string_t::to_std() const {
    return std::string(data(), size());
}

datum_string_t concat(const datum_string_t &a, const datum_string_t &b) {
    const size_t a_size = a.size();
    const size_t b_size = b.size();
    const size_t str_offset = varint_uint64_serialized_size(a_size + b_size);
    counted_t<shared_buf_t> buf = shared_buf_t::create(str_offset + a_size + b_size);
    serialize_varint_uint64_into_buf(a_size + b_size,
                                     reinterpret_cast<uint8_t *>(buf->data(0)));
    memcpy(buf->data(str_offset), a.data(), a.size());
    memcpy(buf->data(str_offset + a.size()), b.data(), b.size());
    return datum_string_t(shared_buf_ref_t<char>(std::move(buf), 0));
}


void debug_print(printf_buffer_t *buf, const datum_string_t &s) {
    debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(s.data()),
                              s.size());
}
