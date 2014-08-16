// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/wire_string.hpp"

// TODO! Clean up includes
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils.hpp"

wire_string_t::wire_string_t()
    : size_(0),
      data_(counted_t<const shared_buf_t>(shared_buf_t::create(0).release()), 0) { }

wire_string_t::wire_string_t(size_t _size, const char *_data)
    : size_(_size),
      data_(counted_t<const shared_buf_t>(
          shared_buf_t::create_and_init(_size, _data).release()), 0) { }

wire_string_t::wire_string_t(size_t _size, const shared_buf_ref_t &_ref)
    : size_(_size),
      data_(_ref) { }

wire_string_t::wire_string_t(size_t _size, shared_buf_ref_t &&_ref)
    : size_(_size),
      data_(std::move(_ref)) { }

wire_string_t::wire_string_t(const char *c_str)
    : size_(strlen(c_str)),
      data_(counted_t<const shared_buf_t>(
          shared_buf_t::create_and_init(size_, c_str).release()), 0) { }

wire_string_t::wire_string_t(const std::string &str)
    : size_(str.size()),
      data_(counted_t<const shared_buf_t>(
          shared_buf_t::create_and_init(str.size(), str.data()).release()), 0) { }

wire_string_t::wire_string_t(wire_string_t &&movee) noexcept
    : size_(movee.size_),
      data_(std::move(movee.data_)) { }

wire_string_t::wire_string_t(const wire_string_t &copyee) noexcept
    : size_(copyee.size_),
      data_(copyee.data_) { }

wire_string_t &wire_string_t::operator=(wire_string_t &&movee) noexcept {
    size_ = movee.size_;
    data_ = std::move(movee.data_);
    return *this;
}

wire_string_t &wire_string_t::operator=(const wire_string_t &copyee) noexcept {
    size_ = copyee.size_;
    data_ = copyee.data_;
    return *this;
}

const char *wire_string_t::data() const {
    return data_.get();
}
size_t wire_string_t::size() const {
    return size_;
}
bool wire_string_t::empty() const {
    return size_ == 0;
}

int wire_string_t::compare(const wire_string_t &other) const {
    size_t common_size = std::min(size_, other.size_);
    int content_compare = memcmp(data_.get(), other.data_.get(), common_size);
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
    return strncmp(data_.get(), other, size_) == 0;
}
bool wire_string_t::operator!=(const char *other) const {
    return !(*this == other);
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
    return std::string(data_.get(), size_);
}

wire_string_t concat(const wire_string_t &a, const wire_string_t &b) {
    scoped_ptr_t<shared_buf_t> buf = shared_buf_t::create(a.size() + b.size());
    memcpy(buf->data(0), a.data(), a.size());
    memcpy(buf->data(a.size()), b.data(), b.size());
    return wire_string_t(
        a.size() + b.size(),
        shared_buf_ref_t(counted_t<const shared_buf_t>(buf.release()), 0));
}
