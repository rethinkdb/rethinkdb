#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "utils2.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/node.hpp"

class keyvalue_location_t {
public:
    keyvalue_location_t() { }

    friend void find_keyvalue_location_for_write(value_sizer_t *sizer, transaction_t *txn, buf_lock_t& sb_buf, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t *keyvalue_location_out);

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc<value_type_t> value;

    DISABLE_COPYING(keyvalue_location_t);
};

void find_keyvalue_location_for_write(value_sizer_t *sizer, transaction_t *txn, buf_lock_t& sb_buf, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t *keyvalue_location_out);

#endif  // __BTREE_OPERATIONS_HPP__
