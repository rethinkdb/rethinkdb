// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_DEPTH_FIRST_TRAVERSAL_HPP_
#define BTREE_DEPTH_FIRST_TRAVERSAL_HPP_

#include "btree/keys.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "containers/counted.hpp"

class btree_slice_t;
class superblock_t;
class depth_first_traversal_callback_t;

enum class direction_t {
    FORWARD,
    BACKWARD
};


// For the lack of a better name...  This is the value held by a depth first
// traversal callback.  It gets passed back to the depth first traversal function
// when the code no longer needs to hold on to the leaf node block, or the buffer
// containing the value.  It's noncopyable to prevent misuse: the depth first
// traversal code needs to know that when it gets passed the value back, no rogue
// references are held to the buf_lock_t.
class dft_value_t {
public:
    dft_value_t(const btree_key_t *key, const void *value, const counted_t<counted_buf_lock_t> &buflock);
    ~dft_value_t();
    dft_value_t(dft_value_t &&movee);
    dft_value_t &operator=(dft_value_t &&movee);

    const btree_key_t *key() const { return key_; }
    const void *value() const { return value_; }

    movable_t<counted_buf_lock_t> release_keepalive() {
        return std::move(btree_leaf_keepalive_);
    }

private:
    void swap(dft_value_t &other);

    const btree_key_t *key_;
    const void *value_;
    movable_t<counted_buf_lock_t> btree_leaf_keepalive_;

    DISABLE_COPYING(dft_value_t);
};

class depth_first_traversal_callback_t {
public:
    depth_first_traversal_callback_t() { }

    /* Return value of `true` indicates to keep going; `false` indicates to stop
    traversing the tree. */
    virtual bool handle_pair(dft_value_t &&keyvalue) = 0;

protected:
    virtual ~depth_first_traversal_callback_t() { }

private:
    DISABLE_COPYING(depth_first_traversal_callback_t);
};


/* Returns `true` if we reached the end of the btree or range, and `false` if
`cb->handle_value()` returned `false`. */
bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction,
        superblock_t *superblock, const key_range_t &range,
        depth_first_traversal_callback_t *cb, direction_t direction);


#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
