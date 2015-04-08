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

class counted_buf_lock_and_read_t :
    public single_threaded_countable_t<counted_buf_lock_and_read_t> {
public:
    template <class... Args>
    explicit counted_buf_lock_and_read_t(Args &&... args)
        : lock(std::forward<Args>(args)...) { }
    buf_lock_t lock;
    scoped_ptr_t<buf_read_t> read;
};

// A btree leaf key/value pair that also owns a reference to the buf_lock_t that
// contains said key/value pair.
class scoped_key_value_t {
public:
    scoped_key_value_t(const btree_key_t *key,
                       const void *value,
                       movable_t<counted_buf_lock_and_read_t> &&buf);
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
    movable_t<counted_buf_lock_and_read_t> buf_;

    DISABLE_COPYING(scoped_key_value_t);
};

class depth_first_traversal_callback_t {
public:
    /* Any of these methods can abort the traversal by returning `ABORT`. In this case,
    no further method calls will happen and `btree_depth_first_traversal()` will return
    `ABORT`. */

    /* Can be overloaded if you don't want to query a contiguous range of keys, but only
    parts of it. Both `filter_range()` and `filter_range_ts()` will be called before
    traversing into any child node; if either sets `*skip_out` to `true`, the range
    will be ignored. */
    virtual continue_bool_t filter_range(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }
    virtual continue_bool_t filter_range_ts(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            UNUSED repli_timestamp_t timestamp,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }

    /* Called on every leaf node before the calls to `handle_pair()`. If it sets
    `*skip_out` to `true`, the leaf will be ignored. */
    virtual continue_bool_t handle_pre_leaf(
            UNUSED const counted_t<counted_buf_lock_and_read_t> &buf,
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }

    /* The guts of the callback; this handles the actual key-value pairs. */
    virtual continue_bool_t handle_pair(scoped_key_value_t &&keyvalue) = 0;

    virtual profile::trace_t *get_trace() THROWS_NOTHING { return NULL; }
protected:
    virtual ~depth_first_traversal_callback_t() { }
};

enum direction_t {
    FORWARD,
    BACKWARD
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(direction_t, int8_t, FORWARD, BACKWARD);

/* Returns `CONTINUE` if we reached the end of the btree or range, and `ABORT` if
`cb->handle_value()` returned `ABORT`. */
continue_bool_t btree_depth_first_traversal(
    superblock_t *superblock,
    const key_range_t &range,
    depth_first_traversal_callback_t *cb,
    access_t access,
    direction_t direction,
    release_superblock_t release_superblock,
    signal_t *interruptor);

#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
