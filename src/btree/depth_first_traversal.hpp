// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_DEPTH_FIRST_TRAVERSAL_HPP_
#define BTREE_DEPTH_FIRST_TRAVERSAL_HPP_

#include "btree/keys.hpp"
#include "btree/types.hpp"
#include "buffer_cache/alt.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "repli_timestamp.hpp"

namespace profile { class trace_t; }

class buf_parent_t;
class superblock_t;

class counted_buf_lock_t : public buf_lock_t,
                           public single_threaded_countable_t<counted_buf_lock_t> {
public:
    template <class... Args>
    explicit counted_buf_lock_t(Args &&... args)
        : buf_lock_t(std::forward<Args>(args)...) { }
};

class counted_buf_read_t : public buf_read_t,
                           public single_threaded_countable_t<counted_buf_read_t> {
public:
    template <class... Args>
    explicit counted_buf_read_t(Args &&... args)
        : buf_read_t(std::forward<Args>(args)...) { }
};

// A btree leaf key/value pair that also owns a reference to the buf_lock_t that
// contains said key/value pair.
class scoped_key_value_t {
public:
    scoped_key_value_t(const btree_key_t *key,
                       const void *value,
                       movable_t<counted_buf_lock_t> &&buf,
                       movable_t<counted_buf_read_t> &&read);
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
    movable_t<counted_buf_read_t> read_;

    DISABLE_COPYING(scoped_key_value_t);
};

class depth_first_traversal_callback_t {
public:
    /* Return value of `NO` indicates to keep going; `YES` indicates to stop
    traversing the tree. */
    virtual continue_bool_t handle_pair(scoped_key_value_t &&keyvalue) = 0;

    /* Called on every leaf node before the calls to `handle_pair()`. If it sets
    `*skip_out` to `true`, no calls to `handle_pair()` will be generated for the leaf. */
    virtual continue_bool_t handle_pre_leaf(
            UNUSED const counted_t<counted_buf_lock_t> &buf_lock,
            UNUSED const counted_t<counted_buf_read_t> &buf_read,
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    } 

    /* Can be overloaded if you don't want to query a contiguous range of keys, but only
    parts of it. Both `is_range_interesting()` and `is_range_ts_interesting()` will be
    called before traversing into any child node; if either returns `false`, the range
    will be ignored. Note: returning false here does not guarantee that a given range is
    never encountered by handle_pair(); it's just a pre-filter. */
    virtual bool is_range_interesting(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl) {
        return true;
    }
    virtual bool is_range_ts_interesting(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            UNUSED repli_timestamp_t timestamp) {
        return true;
    }
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
continue_bool_t btree_depth_first_traversal(
    superblock_t *superblock,
    const key_range_t &range,
    depth_first_traversal_callback_t *cb,
    direction_t direction,
    release_superblock_t release_superblock);

#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
