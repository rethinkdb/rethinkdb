// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_TYPES_HPP_
#define BTREE_TYPES_HPP_

#include "btree/keys.hpp"
#include "buffer_cache/alt/alt.hpp" // for buf_parent_t and buf_lock_t
#include "containers/counted.hpp"

class buf_parent_t;

class counted_buf_lock_t : public buf_lock_t,
                           public single_threaded_countable_t<counted_buf_lock_t> {
public:
    template <class... Args>
    explicit counted_buf_lock_t(Args &&... args)
        : buf_lock_t(std::forward<Args>(args)...) { }
};

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

enum class done_traversing_t { NO, YES };

class value_deleter_t {
public:
    value_deleter_t() { }
    virtual void delete_value(buf_parent_t leaf_node, const void *value) const = 0;

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

/* A deleter that does absolutely nothing. */
class noop_value_deleter_t : public value_deleter_t {
public:
    noop_value_deleter_t() { }
    void delete_value(buf_parent_t, const void *) const { }
};

enum class release_superblock_t {RELEASE, KEEP};

#endif /* BTREE_TYPES_HPP_ */
