#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>

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

    void swap(keyvalue_location_t& other) {
        txn.swap(other.txn);
        sb_buf.swap(other.sb_buf);
        last_buf.swap(other.last_buf);
        buf.swap(other.buf);
        std::swap(there_originally_was_value, other.there_originally_was_value);
        value.swap(other.value);
    }

private:
    DISABLE_COPYING(keyvalue_location_t);
};

template <class Value>
class value_txn_t {
public:
    value_txn_t();
    value_txn_t(btree_key_t *, keyvalue_location_t<Value>&, repli_timestamp_t);
    value_txn_t(btree_slice_t *slice, btree_key_t *key, const repli_timestamp_t tstamp, const order_token_t token);

    // TODO: Where is this copy constructor implemented and how could
    // this possibly be implemented?
    //value_txn_t(const value_txn_t&);

    void swap(value_txn_t);

    ~value_txn_t();

    scoped_malloc<Value>& value();

    transaction_t *get_txn();
private:
    btree_key_t *key;
    keyvalue_location_t<Value> kv_location;
    repli_timestamp_t tstamp;

    DISABLE_COPYING(value_txn_t<Value>);
};

#include "btree/operations.tcc"

#endif  // __BTREE_OPERATIONS_HPP__
