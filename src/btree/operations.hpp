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
    scoped_malloc<value_type_t> value;

private:
    DISABLE_COPYING(keyvalue_location_t);
};

void find_keyvalue_location_for_write(value_sizer_t *sizer, got_superblock_t *got_superblock, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t *keyvalue_location_out);

void find_keyvalue_location_for_read(value_sizer_t *sizer, got_superblock_t *got_superblock, btree_key_t *key, keyvalue_location_t *keyvalue_location_out);

void apply_keyvalue_change(value_sizer_t *sizer, keyvalue_location_t *location_and_value, btree_key_t *key, repli_timestamp_t timestamp);

#endif  // __BTREE_OPERATIONS_HPP__
