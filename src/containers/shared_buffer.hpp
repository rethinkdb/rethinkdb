// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SHARED_BUFFER_HPP_
#define CONTAINERS_SHARED_BUFFER_HPP_

#include <vector>

#include "containers/counted.hpp"
#include "errors.hpp"

/* A `shared_buffer_t` is a reference counted binary buffer.
You can have multiple `shared_buf_ref_t`s pointing at different offsets in
the same `shared_buffer_t`. */
// TODO! Avoid the pointer indirection, similar to how wire_string_t used to work.
class shared_buf_t : public std::vector<char>,
                     public slow_atomic_countable_t<shared_buf_t> {
public:
    explicit shared_buf_t(size_t initial_size)
        : std::vector<char>(initial_size) { }
    explicit shared_buf_t(std::vector<char> &&data)
        : std::vector<char>(std::move(data)) { }
};

/* A `shared_buf_ref_t` points at a specific offset of a `shared_buf_t`.
It is packed to reduce its memory footprint. Additionally you can specify
a smaller offset_t type if you don't need to access buffers of more than a certain
size. */
//template<class offset_t = uint64_t> // TODO!
class shared_buf_ref_t {
public:
    shared_buf_ref_t(const counted_t<const shared_buf_t> &_buf, uint64_t _offset)
        : buf(_buf), offset(_offset) {
        rassert(buf.has());
        rassert(static_cast<size_t>(offset) <= buf->size());
    }
    shared_buf_ref_t(shared_buf_ref_t &&movee) noexcept
        : buf(std::move(movee.buf)), offset(movee.offset) { }
    shared_buf_ref_t(const shared_buf_ref_t &copyee) noexcept
        : buf(copyee.buf), offset(copyee.offset) { }
    shared_buf_ref_t &operator=(shared_buf_ref_t &&movee) noexcept {
        buf = std::move(movee.buf);
        offset = movee.offset;
        return *this;
    }
    shared_buf_ref_t &operator=(const shared_buf_ref_t &copyee) noexcept {
        buf = copyee.buf;
        offset = copyee.offset;
        return *this;
    }
    const char *get() const {
        return &(*buf)[offset];
    }
private:
    counted_t<const shared_buf_t> buf;
    uint64_t offset;
} __attribute__((__packed__));


#endif  // CONTAINERS_SHARED_BUFFER_HPP_
