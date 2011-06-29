#ifndef __BTREE_MODIFY_OPER_HPP__
#define __BTREE_MODIFY_OPER_HPP__

#include "utils.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/co_functions.hpp"
#include "containers/scoped_malloc.hpp"

#define BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS 0

/* Stats */
class btree_modify_oper_t {
public:
    btree_modify_oper_t() : slice(NULL) { }

    virtual ~btree_modify_oper_t() { }

    // run_btree_modify_oper() calls operate() when it reaches the
    // leaf node.  It modifies the value of (or the existence of
    // `value` in some way.  For example, if value contains a NULL
    // pointer, that means no such key-value pair exists.  Setting the
    // value to NULL would mean to delete the key-value pair (but if
    // you do so make sure to wipe out the blob, too).  The return
    // value is true if the leaf node needs to be updated.
    virtual bool operate(transaction_t *txn, scoped_malloc<btree_value_t>& value) = 0;


    virtual int compute_expected_change_count(const size_t block_size) = 0;

    // These two variables are only used by the get_cas_oper; there should be a
    // nicer way to handle this.
    btree_slice_t *slice;

    // This is a dorky name.  This function shall be called
    // immediately after the superblock has been acquired.  The delete
    // queue is a child of the superblock, when it comes to
    // transactional ordering.
    virtual void do_superblock_sidequest(UNUSED transaction_t *txn,
                                         UNUSED buf_lock_t& superblock,
                                         UNUSED repli_timestamp recency,
                                         UNUSED const store_key_t *key) { }
};

// Runs a btree_modify_oper_t.
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &key, castime_t castime, order_token_t token);

buf_t *get_root(transaction_t *txn, buf_t **sb_buf, block_size_t block_size);
void insert_root(block_id_t root_id, buf_t **sb_buf);
void check_and_handle_split(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
    const btree_key_t *key, btree_value_t *new_value, block_size_t block_size);
void check_and_handle_underfull(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
    const btree_key_t *key, block_size_t block_size);


#endif // __BTREE_MODIFY_OPER_HPP__
