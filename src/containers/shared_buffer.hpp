// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SHARED_BUFFER_HPP_
#define CONTAINERS_SHARED_BUFFER_HPP_

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"

/* A `shared_buffer_t` is a reference counted binary buffer.
You can have multiple `shared_buf_ref_t`s pointing to different offsets in
the same `shared_buffer_t`. */
class shared_buf_t {
public:
    shared_buf_t() = delete;

    static scoped_ptr_t<shared_buf_t> create(size_t _size);
    static scoped_ptr_t<shared_buf_t> create_and_init(size_t _size, const char *_data);
    static void operator delete(void *p);

    char *data(size_t offset = 0);
    const char *data(size_t offset = 0) const;

private:
    // We duplicate the implementation of slow_atomic_countable_t here for the
    // sole purpose of having full control over the layout of fields. This
    // is required because we manually allocate memory for the data field,
    // and C++ doesn't guarantee any specific field memory layout under inheritance
    // (as far as I know).
    friend void counted_add_ref(const shared_buf_t *p);
    friend void counted_release(const shared_buf_t *p);
    friend intptr_t counted_use_count(const shared_buf_t *p);

    mutable intptr_t refcount_;
    // We actually allocate more memory than this.
    // It's crucial that this field is the last one in this class.
    char data_[1];

    DISABLE_COPYING(shared_buf_t);
};


/* A `shared_buf_ref_t` points at a specific offset of a `shared_buf_t`.
It is packed to reduce its memory footprint. Additionally you can specify
a smaller offset_t type if you don't need to access buffers of more than a certain
size. */
// TODO (daniel): This should be templated on the offset type, so we can
// save some memory when having buffers of a limited maximum size.
class shared_buf_ref_t {
public:
    shared_buf_ref_t(const counted_t<const shared_buf_t> &_buf, uint64_t _offset)
        : buf(_buf), offset(_offset) {
        rassert(buf.has());
    }
    shared_buf_ref_t(shared_buf_ref_t &&movee) noexcept
        : buf(std::move(movee.buf)), offset(movee.offset) {
        rassert(buf.has());
    }
    shared_buf_ref_t(const shared_buf_ref_t &copyee) noexcept
        : buf(copyee.buf), offset(copyee.offset) {
        rassert(buf.has());
    }
    shared_buf_ref_t &operator=(shared_buf_ref_t &&movee) noexcept {
        buf = std::move(movee.buf);
        offset = movee.offset;
        rassert(buf.has());
        return *this;
    }
    shared_buf_ref_t &operator=(const shared_buf_ref_t &copyee) noexcept {
        buf = copyee.buf;
        offset = copyee.offset;
        rassert(buf.has());
        return *this;
    }
    const char *get() const {
        rassert(buf.has());
        return buf->data(offset);
    }
private:
    counted_t<const shared_buf_t> buf;
    uint64_t offset;
} __attribute__((__packed__));


inline void counted_add_ref(const shared_buf_t *p) {
    DEBUG_VAR intptr_t res = __sync_add_and_fetch(&p->refcount_, 1);
    rassert(res > 0);
}

inline void counted_release(const shared_buf_t *p) {
    intptr_t res = __sync_sub_and_fetch(&p->refcount_, 1);
    rassert(res >= 0);
    if (res == 0) {
        delete const_cast<shared_buf_t *>(p);
    }
}

inline intptr_t counted_use_count(const shared_buf_t *p) {
    // Finally a practical use for volatile.
    intptr_t tmp = static_cast<const volatile intptr_t&>(p->refcount_);
    rassert(tmp > 0);
    return tmp;
}

#endif  // CONTAINERS_SHARED_BUFFER_HPP_
