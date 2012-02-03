#include "btree/delete.hpp"
#include "btree/modify_oper.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    btree_delete_oper_t(bool dont_record, btree_slice_t *_slice) : dont_put_in_delete_queue(dont_record), slice(_slice) { }

    delete_result_t result;
    bool dont_put_in_delete_queue;
    btree_slice_t *slice;

    bool operate(transaction_t *txn, scoped_malloc<memcached_value_t>& value) {
        if (value) {
            result = dr_deleted;
            {
                blob_t b(value->value_ref(), blob::btree_maxreflen);
                b.clear(txn);
            }
            value.reset();
            return true;
        } else {
            result = dr_not_found;
            return false;
        }
    }

    int compute_expected_change_count(UNUSED block_size_t block_size) {
        return 1;
    }
};

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, sequence_group_t *seq_group, repli_timestamp_t timestamp, order_token_t token) {
    btree_delete_oper_t oper(dont_put_in_delete_queue, slice);
    run_btree_modify_oper(&oper, slice, seq_group, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp), token);
    return oper.result;
}

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, repli_timestamp_t timestamp,
    transaction_t *txn, got_superblock_t& superblock) {

    btree_delete_oper_t oper(dont_put_in_delete_queue, slice);
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp), txn, superblock);
    return oper.result;
}

