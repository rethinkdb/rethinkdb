#include "memcached/memcached_btree/delete.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"
#include "repli_timestamp.hpp"

struct memcached_delete_oper_t : public memcached_modify_oper_t {

    memcached_delete_oper_t(bool dont_record, btree_slice_t *_slice) : dont_put_in_delete_queue(dont_record), slice(_slice) { }

    delete_result_t result;
    bool dont_put_in_delete_queue;
    btree_slice_t *slice;

    bool operate(transaction_t *txn, scoped_malloc_t<memcached_value_t> *value) {
        if (value->has()) {
            result = dr_deleted;
            {
                blob_t b((*value)->value_ref(), blob::btree_maxreflen);
                b.clear(txn);
            }
            value->reset();
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

delete_result_t memcached_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, exptime_t effective_time, repli_timestamp_t timestamp,
    transaction_t *txn, superblock_t *superblock) {

    memcached_delete_oper_t oper(dont_put_in_delete_queue, slice);
    run_memcached_modify_oper(&oper, slice, key, BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, effective_time, timestamp, txn, superblock);
    return oper.result;
}

