#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>

#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

class btree_slice_t;

/* An abstract superblock provides the starting point for performing btree operations */
class superblock_t {
public:
    superblock_t() { }
    virtual ~superblock_t() { }
    // Release the superblock if possible (otherwise do nothing)
    virtual void release() = 0;
    // If we hold a lock on a super block, swap it into swapee
    // (might swap in an empty buf_lock_t if we don't have an actual superblock)
    virtual void swap_buf(buf_lock_t &swapee) = 0;
    virtual block_id_t get_root_block_id() const = 0;
    virtual void set_root_block_id(const block_id_t new_root_block) = 0;

private:
    DISABLE_COPYING(superblock_t);
};

/* real_superblock_t implements superblock_t in terms of an actual on-disk block
   structure. */
class real_superblock_t : public superblock_t {
public:
    real_superblock_t(buf_lock_t &sb_buf);

    void release();
    void swap_buf(buf_lock_t &swapee);
    block_id_t get_root_block_id() const;
    void set_root_block_id(const block_id_t new_root_block);
    block_id_t get_delete_queue_block() const;

private:
    buf_lock_t sb_buf_;
};

/* This is for nested btrees, where the "superblock" is really more like a super value.
 It provides an in-memory superblock replacement.

 Note for use for nested btrees: If you want to nest a tree into some super value,
 you would probably have a block_id_t nested_root value in the super value. Then,
 before accessing the nested tree, you can construct a virtual_superblock_t
 based on the nested_root value. Once write operations to the nested btree have
 finished, you should check whether the root_block_id has been changed,
 and if it has, use get_root_block_id() to update the nested_root value in the
 super block.
 */
class virtual_superblock_t : public superblock_t {
public:
    virtual_superblock_t(block_id_t root_block_id = NULL_BLOCK_ID) : root_block_id_(root_block_id) { }

    void release() { }
    void swap_buf(buf_lock_t &swapee) {
        // Swap with empty buf_lock
        buf_lock_t tmp;
        tmp.swap(swapee);
    }
    block_id_t get_root_block_id() const {
        return root_block_id_;
    }
    void set_root_block_id(const block_id_t new_root_block) {
        root_block_id_ = new_root_block;
    }
    block_id_t get_delete_queue_block() const {
        return NULL_BLOCK_ID;
    }

private:
    block_id_t root_block_id_;
};

class got_superblock_t {
public:
    got_superblock_t() { }

    boost::scoped_ptr<superblock_t> sb;

private:
    DISABLE_COPYING(got_superblock_t);
};

template <class Value>
class keyvalue_location_t {
public:
    keyvalue_location_t() : there_originally_was_value(false) { }

    boost::scoped_ptr<superblock_t> sb;

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc<Value> value;

    void swap(keyvalue_location_t& other) {
        sb.swap(other.sb);
        last_buf.swap(other.last_buf);
        buf.swap(other.buf);
        std::swap(there_originally_was_value, other.there_originally_was_value);
        value.swap(other.value);
    }

private:
    DISABLE_COPYING(keyvalue_location_t);
};



template <class Value>
class key_modification_callback_t {
public:
    // Perhaps this modifies the kv_loc in place, swapping in its own
    // scoped_malloc.  It's the caller's responsibility to have
    // destroyed any blobs that the value might reference, before
    // calling this here, so that this callback can reacquire them.
    virtual key_modification_proof_t value_modification(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, const btree_key_t *key) = 0;

    key_modification_callback_t() { }
protected:
    virtual ~key_modification_callback_t() { }
private:
    DISABLE_COPYING(key_modification_callback_t);
};


template <class Value>
class value_txn_t {
public:
    value_txn_t(btree_key_t *, keyvalue_location_t<Value>&, repli_timestamp_t, key_modification_callback_t<Value> *km_callback);
    value_txn_t(btree_slice_t *slice, btree_key_t *key, const repli_timestamp_t tstamp, const order_token_t token, key_modification_callback_t<Value> *km_callback);

    ~value_txn_t();

    scoped_malloc<Value>& value();

    transaction_t *get_txn();

private:
    btree_key_t *key;
    boost::scoped_ptr<transaction_t> txn;
    keyvalue_location_t<Value> kv_location;
    repli_timestamp_t tstamp;

    // Not owned by this object.
    key_modification_callback_t<Value> *km_callback;

    DISABLE_COPYING(value_txn_t);
};



template <class Value>
class null_key_modification_callback_t : public key_modification_callback_t<Value> {
    key_modification_proof_t value_modification(UNUSED transaction_t *txn, UNUSED keyvalue_location_t<Value> *kv_loc, UNUSED const btree_key_t *key) {
        // do nothing
        return key_modification_proof_t::real_proof();
    }
};

// TODO: Remove all instances of this, each time considering what kind
// of key modification callback is necessary.
template <class Value>
class fake_key_modification_callback_t : public key_modification_callback_t<Value> {
    key_modification_proof_t value_modification(UNUSED transaction_t *txn, UNUSED keyvalue_location_t<Value> *kv_loc, UNUSED const btree_key_t *key) {
        // do nothing
        return key_modification_proof_t::fake_proof();
    }
};


#include "btree/operations.tcc"

#endif  // __BTREE_OPERATIONS_HPP__
