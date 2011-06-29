#include "btree/delete.hpp"
#include "btree/modify_oper.hpp"

#include "replication/delete_queue.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    btree_delete_oper_t(bool dont_record) : dont_put_in_delete_queue(dont_record) { }

    delete_result_t result;
    bool dont_put_in_delete_queue;

    bool operate(const boost::shared_ptr<transaction_t>& txn, scoped_malloc<btree_value_t>& value) {
        if (value) {
            result = dr_deleted;
            {
                blob_t b(value->value_ref(), blob::btree_maxreflen);
                b.unappend_region(txn.get(), b.valuesize());
            }
            value.reset();
            return true;
        } else {
            result = dr_not_found;
            return false;
        }
    }

    int compute_expected_change_count(UNUSED const size_t block_size) {
        return 1;
    }

    void do_superblock_sidequest(boost::shared_ptr<transaction_t>& txn,
                                 buf_lock_t& superblock,
                                 repli_timestamp recency,
                                 const store_key_t *key) {

        if (!dont_put_in_delete_queue) {
            slice->assert_thread();
            const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());

            replication::add_key_to_delete_queue(slice->delete_queue_limit(), txn, sb->delete_queue_block, recency, key);
        }
    }
};

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, repli_timestamp timestamp, order_token_t token) {
    btree_delete_oper_t oper(dont_put_in_delete_queue);
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp), token);
    return oper.result;
}
