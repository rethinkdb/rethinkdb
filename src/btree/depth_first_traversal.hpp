// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_DEPTH_FIRST_TRAVERSAL_HPP_
#define BTREE_DEPTH_FIRST_TRAVERSAL_HPP_

#include "btree/keys.hpp"
#include "btree/types.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"

namespace profile { class trace_t; }

class buf_parent_t;
class counted_buf_lock_t;
class superblock_t;

// A btree leaf key/value pair that also owns a reference to the buf_lock_t that
// contains said key/value pair.
class scoped_key_value_t {
public:
    scoped_key_value_t(const btree_key_t *key,
                       const void *value,
                       movable_t<counted_buf_lock_t> &&buf);
    scoped_key_value_t(scoped_key_value_t &&movee);
    ~scoped_key_value_t();
    void operator=(scoped_key_value_t &&) = delete;

    const btree_key_t *key() const {
        guarantee(buf_.has());
        return key_;
    }
    const void *value() const {
        guarantee(buf_.has());
        return value_;
    }
    buf_parent_t expose_buf();

    // Releases the hold on the buf_lock_t, after which key(), value(), and
    // expose_buf() may not be used.
    void reset();

private:
    const btree_key_t *key_;
    const void *value_;
    movable_t<counted_buf_lock_t> buf_;

    DISABLE_COPYING(scoped_key_value_t);
};

class depth_first_traversal_callback_t {
public:
    /* Return value of `NO` indicates to keep going; `YES` indicates to stop
    traversing the tree. */
    virtual done_traversing_t handle_pair(scoped_key_value_t &&keyvalue) = 0;
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
bool btree_depth_first_traversal(superblock_t *superblock,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction,
                                 release_superblock_t release_superblock
                                     = release_superblock_t::RELEASE);

#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
