#ifndef __BTREE_MODIFY_OPER_HPP__
#define __BTREE_MODIFY_OPER_HPP__

#include "btree/node.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/operations.hpp"

class btree_slice_t;

#define BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS 0

/* Stats */
class btree_modify_oper_t {
protected:
    virtual ~btree_modify_oper_t() { }

public:
    // run_btree_modify_oper() calls operate() when it reaches the
    // leaf node.  It modifies the value of (or the existence of
    // `value` in some way.  For example, if value contains a NULL
    // pointer, that means no such key-value pair exists.  Setting the
    // value to NULL would mean to delete the key-value pair (but if
    // you do so make sure to wipe out the blob, too).  The return
    // value is true if the leaf node needs to be updated.
    virtual MUSTUSE bool operate(transaction_t *txn, scoped_malloc<memcached_value_t>& value) = 0;


    virtual MUSTUSE int compute_expected_change_count(block_size_t block_size) = 0;
};

// Runs a btree_modify_oper_t.
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &key, castime_t castime, order_token_t token);
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &key, castime_t castime, order_token_t token,
    const boost::scoped_ptr<transaction_t>& txn, got_superblock_t& superblock);


#endif // __BTREE_MODIFY_OPER_HPP__
