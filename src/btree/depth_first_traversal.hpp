// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_DEPTH_FIRST_TRAVERSAL_HPP_
#define BTREE_DEPTH_FIRST_TRAVERSAL_HPP_

#include "btree/keys.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "containers/archive/archive.hpp"

class superblock_t;

namespace profile { class trace_t; }

class counted_buf_lock_t : public alt::alt_buf_lock_t,
                           public single_threaded_countable_t<counted_buf_lock_t> {
public:
    template <class... Args>
    explicit counted_buf_lock_t(Args &&... args)
        : alt::alt_buf_lock_t(std::forward<Args>(args)...) { }
};

// A btree leaf key/value pair that also owns a reference to the alt_buf_lock_t that
// contains said key/value pair.
class scoped_key_value_t {
public:
    scoped_key_value_t(const btree_key_t *key,
                       const void *value,
                       movable_t<counted_buf_lock_t> &&buf)
        : key_(key), value_(value), buf_(std::move(buf)) {
        guarantee(buf_.has());
    }

    scoped_key_value_t(scoped_key_value_t &&movee)
        : key_(movee.key_),
          value_(movee.value_),
          buf_(std::move(movee.buf_)) {
        movee.key_ = NULL;
        movee.value_ = NULL;
    }

    const btree_key_t *key() const {
        guarantee(buf_.has());
        return key_;
    }
    const void *value() const {
        guarantee(buf_.has());
        return value_;
    }
    alt::alt_buf_parent_t expose_buf() {
        guarantee(buf_.has());
        return alt::alt_buf_parent_t(buf_.get());
    }

    // Releases the hold on the alt_buf_lock_t, after which key(), value(), and
    // expose_buf() may not be used.
    void reset() { buf_.reset(); }

private:
    const btree_key_t *key_;
    const void *value_;
    movable_t<counted_buf_lock_t> buf_;
};

class depth_first_traversal_callback_t {
public:
    /* Return value of `true` indicates to keep going; `false` indicates to stop
    traversing the tree. */
    virtual bool handle_pair(scoped_key_value_t &&keyvalue) = 0;
    virtual profile::trace_t *get_trace() THROWS_NOTHING { return NULL; }
protected:
    virtual ~depth_first_traversal_callback_t() { }
};

enum direction_t {
    FORWARD,
    BACKWARD
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(direction_t, int8_t, FORWARD, BACKWARD);

/* Returns `true` if we reached the end of the btree or range, and `false` if
`cb->handle_value()` returned `false`. */
bool btree_depth_first_traversal(btree_slice_t *slice, superblock_t *superblock,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction);

#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
