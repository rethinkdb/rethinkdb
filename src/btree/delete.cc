#include "btree/delete.hpp"
#include "btree/modify_oper.hpp"

#include "replication/delete_queue.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    btree_delete_oper_t(bool dont_record) : dont_put_in_delete_queue(dont_record) { }

    delete_result_t result;
    bool dont_put_in_delete_queue;

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

    void do_superblock_sidequest(boost::shared_ptr<transactor_t>& txor,
                                 buf_lock_t& superblock,
                                 repli_timestamp recency,
                                 const store_key_t *key) {

        if (!dont_put_in_delete_queue) {
            slice->assert_thread();
            const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());

            replication::add_key_to_delete_queue(txor, sb->delete_queue_block, recency, key);
        }
    }
};

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, repli_timestamp timestamp) {
    btree_delete_oper_t oper(dont_put_in_delete_queue);
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp));
    return oper.result;
}
