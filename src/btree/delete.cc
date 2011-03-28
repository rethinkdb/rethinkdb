#include "btree/delete.hpp"
#include "btree/modify_oper.hpp"

#include "replication/delete_queue.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    delete_result_t result;

    bool operate(UNUSED const boost::shared_ptr<transactor_t>& txor, btree_value *old_value, UNUSED boost::scoped_ptr<large_buf_t>& old_large_buflock, btree_value **new_value, UNUSED boost::scoped_ptr<large_buf_t>& new_large_buflock) {
        if (old_value) {
            result = dr_deleted;
            *new_value = NULL;
            return true;
        } else {
            result = dr_not_found;
            return false;
        }
    }

    int compute_expected_change_count(UNUSED const size_t block_size) {
        return 1;
    }

    // You must have the btree superblock acquired when calling this
    // function.
    void do_superblock_sidequest(boost::shared_ptr<transactor_t>& txor,
                                 UNUSED buf_lock_t& superblock,
                                 repli_timestamp recency,
                                 const store_key_t *key) {
        slice->assert_thread();
        // TODO: Consider just making the superblock store the delete
        // queue block -- the treatment of transactional behavior is
        // less error-prone that way.
        replication::add_key_to_delete_queue(txor, DELETE_QUEUE_ID, recency, key);
    }
};

delete_result_t btree_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp timestamp) {
    btree_delete_oper_t oper;
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp));
    return oper.result;
}
