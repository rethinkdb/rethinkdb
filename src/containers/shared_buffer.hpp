// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SHARED_BUFFER_HPP_
#define CONTAINERS_SHARED_BUFFER_HPP_

#include <atomic>

#include "containers/counted.hpp"
#include "errors.hpp"

/* A `shared_buffer_t` is a reference counted binary buffer.
You can have multiple `shared_buf_ref_t`s pointing to different offsets in
the same `shared_buffer_t`. */
class shared_buf_t {
public:
    shared_buf_t() = delete;

    static counted_t<shared_buf_t> create(size_t _size);
    static void operator delete(void *p);

    char *data(size_t offset = 0);
    const char *data(size_t offset = 0) const;

    size_t size() const;

private:
    // We duplicate the implementation of slow_atomic_countable_t here for the
    // sole purpose of having full control over the layout of fields. This
    // is required because we manually allocate memory for the data field,
    // and C++ doesn't guarantee any specific field memory layout under inheritance
    // (as far as I know).
    friend void counted_add_ref(const shared_buf_t *p);
    friend void counted_release(const shared_buf_t *p);
    friend intptr_t counted_use_count(const shared_buf_t *p);

    mutable std::atomic<intptr_t> refcount_;

    // The size of data_, for boundary checking.
    size_t size_;

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
template <class T>
class shared_buf_ref_t {
public:
    shared_buf_ref_t() : offset(0) { }
    shared_buf_ref_t(const counted_t<const shared_buf_t> &_buf, size_t _offset)
        : buf(_buf), offset(_offset) {
        rassert(buf.has());
    }

    const T *get() const {
        rassert(buf.has());
        rassert(buf->size() >= offset);
        return reinterpret_cast<const T *>(buf->data(offset));
    }

    shared_buf_ref_t make_child(size_t relative_offset) const {
        guarantee_in_boundary(relative_offset);
        return shared_buf_ref_t(buf, offset + relative_offset);
    }

    // Makes sure that the underlying shared buffer has space for at least
    // num_elements elements of type T.
    // This protects against reading into memory that doesn't belong to the
    // buffer, but doesn't protect from reading into another object in case the
    // buffer contains other objects in addition to what this buf ref is pointing
    // to.
    void guarantee_in_boundary(size_t num_elements) const {
        guarantee(get_safety_boundary() >= num_elements);
    }
    // An upper bound on the number of elements that can be read from this buf ref
    size_t get_safety_boundary() const {
        rassert(buf.has());
        rassert(buf->size() >= offset);
        return (buf->size() - offset) / sizeof(T);
    }

private:
    counted_t<const shared_buf_t> buf;
    size_t offset;
};



inline void counted_add_ref(const shared_buf_t *p) {
    DEBUG_VAR intptr_t res = ++(p->refcount_);
    rassert(res > 0);
}

inline void counted_release(const shared_buf_t *p) {
    int64_t res = --(p->refcount_);
    rassert(res >= 0);
    if (res == 0) {
        delete const_cast<shared_buf_t *>(p);
    }
}

inline intptr_t counted_use_count(const shared_buf_t *p) {
    // Finally a practical use for volatile.
    intptr_t tmp = p->refcount_.load();
    rassert(tmp > 0);
    return tmp;
}

#endif  // CONTAINERS_SHARED_BUFFER_HPP_
