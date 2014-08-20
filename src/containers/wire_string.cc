// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/wire_string.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <limits>

#include "containers/archive/archive.hpp"
#include "containers/archive/varint.hpp"
#include "containers/scoped.hpp"
#include "utils.hpp"

wire_string_t::wire_string_t() {
    const size_t str_offset = varint_uint64_serialized_size(0);
    scoped_ptr_t<shared_buf_t> data = shared_buf_t::create(str_offset);
    serialize_varint_uint64_into_buf(0, reinterpret_cast<uint8_t *>(data->data()));
    data_ = shared_buf_ref_t<char>(counted_t<shared_buf_t>(data.release()), 0);
}

wire_string_t::wire_string_t(size_t _size, const char *_data) {
    const size_t str_offset = varint_uint64_serialized_size(_size);
    scoped_ptr_t<shared_buf_t> data = shared_buf_t::create(str_offset + _size);
    serialize_varint_uint64_into_buf(_size, reinterpret_cast<uint8_t *>(data->data()));
    memcpy(data->data() + str_offset, _data, _size);
    data_ = shared_buf_ref_t<char>(counted_t<shared_buf_t>(data.release()), 0);
}

wire_string_t::wire_string_t(const shared_buf_ref_t<char> &_ref)
    : data_(_ref) { }

wire_string_t::wire_string_t(shared_buf_ref_t<char> &&_ref)
    : data_(std::move(_ref)) { }

wire_string_t::wire_string_t(const char *c_str) {
    const size_t str_size = strlen(c_str);
    const size_t str_offset = varint_uint64_serialized_size(str_size);
    scoped_ptr_t<shared_buf_t> data = shared_buf_t::create(str_offset + str_size);
    serialize_varint_uint64_into_buf(str_size, reinterpret_cast<uint8_t *>(data->data()));
    memcpy(data->data() + str_offset, c_str, str_size);
    data_ = shared_buf_ref_t<char>(counted_t<shared_buf_t>(data.release()), 0);
}

wire_string_t::wire_string_t(const std::string &str) {
    const size_t str_offset = varint_uint64_serialized_size(str.size());
    scoped_ptr_t<shared_buf_t> data = shared_buf_t::create(str_offset + str.size());
    serialize_varint_uint64_into_buf(str.size(), reinterpret_cast<uint8_t *>(data->data()));
    memcpy(data->data() + str_offset, str.data(), str.size());
    data_ = shared_buf_ref_t<char>(counted_t<shared_buf_t>(data.release()), 0);
}

wire_string_t::wire_string_t(wire_string_t &&movee) noexcept
    : data_(std::move(movee.data_)) { }

wire_string_t::wire_string_t(const wire_string_t &copyee) noexcept
    : data_(copyee.data_) { }

wire_string_t &wire_string_t::operator=(wire_string_t &&movee) noexcept {
    data_ = std::move(movee.data_);
    return *this;
}

wire_string_t &wire_string_t::operator=(const wire_string_t &copyee) noexcept {
    data_ = copyee.data_;
    return *this;
}

const char *wire_string_t::data() const {
    size_t data_offset = varint_uint64_serialized_size(size());
    return data_.get() + data_offset;
}
size_t wire_string_t::size() const {
    uint64_t res;
    guarantee_deserialization(deserialize_varint_uint64_from_buf(
            reinterpret_cast<const uint8_t *>(data_.get()), &res), "wire_string size");
    guarantee(res <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()));
    return static_cast<size_t>(res);
}
bool wire_string_t::empty() const {
    return size() == 0;
}

int wire_string_t::compare(const wire_string_t &other) const {
    const size_t this_size = size();
    const size_t other_size = other.size();
    size_t common_size = std::min(this_size, other_size);
    int content_compare = memcmp(data(), other.data(), common_size);
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

bool wire_string_t::operator==(const char *other) const {
    return strncmp(data(), other, size()) == 0;
}
bool wire_string_t::operator!=(const char *other) const {
    return !(*this == other);
}

bool wire_string_t::operator==(const wire_string_t &other) const {
    if (size() != other.size()) {
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
    return std::string(data(), size());
}

wire_string_t concat(const wire_string_t &a, const wire_string_t &b) {
    const size_t a_size = a.size();
    const size_t b_size = b.size();
    const size_t str_offset = varint_uint64_serialized_size(a_size + b_size);
    scoped_ptr_t<shared_buf_t> buf = shared_buf_t::create(str_offset + a_size + b_size);
    serialize_varint_uint64_into_buf(a_size + b_size,
                                     reinterpret_cast<uint8_t *>(buf->data(0)));
    memcpy(buf->data(str_offset), a.data(), a.size());
    memcpy(buf->data(str_offset + a.size()), b.data(), b.size());
    return wire_string_t(
        shared_buf_ref_t<char>(counted_t<const shared_buf_t>(buf.release()), 0));
}
