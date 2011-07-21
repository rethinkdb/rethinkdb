#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "utils.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/node.hpp"

class btree_slice_t;

class got_superblock_t {
public:
    got_superblock_t() { }

    boost::scoped_ptr<transaction_t> txn;

    buf_lock_t sb_buf;

private:
    DISABLE_COPYING(got_superblock_t);
};

// For read mode operations.
void get_btree_superblock(btree_slice_t *slice, access_t access, order_token_t token, got_superblock_t *got_superblock_out);

void get_btree_superblock(btree_slice_t *slice, access_t access, int expected_change_count, repli_timestamp_t tstamp, order_token_t token, got_superblock_t *got_superblock_out);

template <class Value>
class keyvalue_location_t {
public:
    keyvalue_location_t() : there_originally_was_value(false) { }

    boost::scoped_ptr<transaction_t> txn;
    buf_lock_t sb_buf;

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc<Value> value;

private:
    DISABLE_COPYING(keyvalue_location_t);
};

template <class Value>
void find_keyvalue_location_for_write(value_sizer_t<Value> *sizer, got_superblock_t *got_superblock, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t<Value> *keyvalue_location_out);

template <class Value>
void find_keyvalue_location_for_read(value_sizer_t<Value> *sizer, got_superblock_t *got_superblock, btree_key_t *key, keyvalue_location_t<Value> *keyvalue_location_out);

template <class Value>
void apply_keyvalue_change(value_sizer_t<Value> *sizer, keyvalue_location_t<Value> *location_and_value, btree_key_t *key, repli_timestamp_t timestamp);


template <class Value>
class value_txn_t {
public:
    value_txn_t(btree_key_t *, value_sizer_t<Value> *, keyvalue_location_t<Value> *, repli_timestamp_t);
    value_txn_t(const value_txn_t &);
    ~value_txn_t();
    scoped_malloc<Value> value;
    transaction_t *get_txn();
private:
    btree_key_t *key;
    value_sizer_t<Value> *sizer;
    keyvalue_location_t<Value> *kv_location;
    repli_timestamp_t tstamp;
};

template <class Value>
value_txn_t<Value> get_value_write(btree_slice_t *, btree_key_t *, repli_timestamp_t, order_token_t);

template <class Value>
void get_value_read(btree_slice_t *, btree_key_t *, order_token_t, keyvalue_location_t<Value> *);

#include "btree/operations.tcc"

#endif  // __BTREE_OPERATIONS_HPP__
