#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "utils.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/node.hpp"
#include "btree/iteration.hpp"

class btree_slice_t;

class got_superblock_t {
public:
    got_superblock_t() { }

    boost::shared_ptr<transaction_t> txn;

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

    boost::shared_ptr<transaction_t> txn;
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
    value_txn_t(btree_key_t *, boost::scoped_ptr<got_superblock_t> &, boost::scoped_ptr<value_sizer_t<Value> > &, boost::scoped_ptr<keyvalue_location_t<Value> > &, repli_timestamp_t);
    value_txn_t(const value_txn_t &);
    ~value_txn_t();
    scoped_malloc<Value> value;

    transaction_t *get_txn();
private:
    btree_key_t *key;
    boost::scoped_ptr<got_superblock_t> got_superblock;
    boost::scoped_ptr<value_sizer_t<Value> > sizer;
    boost::scoped_ptr<keyvalue_location_t<Value> > kv_location;
    repli_timestamp_t tstamp;
};

template <class Value>
value_txn_t<Value> get_value_write(btree_slice_t *, btree_key_t *, const repli_timestamp_t, const order_token_t);

template <class Value>
void get_value_read(btree_slice_t *, btree_key_t *, order_token_t, keyvalue_location_t<Value> *);

template <class Value>
class range_txn_t {
public:
    boost::shared_ptr<transaction_t> txn;
    slice_keys_iterator_t<Value> *it;

public:
    range_txn_t(boost::shared_ptr<transaction_t> &, slice_keys_iterator_t<Value> *);
    transaction_t *get_txn();
    range_txn_t(range_txn_t const&);
};

template <class Value> 
range_txn_t<Value> get_range(btree_slice_t *, order_token_t, rget_bound_mode_t, store_key_t const &, rget_bound_mode_t, store_key_t const &);

#include "btree/operations.tcc"

#endif  // __BTREE_OPERATIONS_HPP__
