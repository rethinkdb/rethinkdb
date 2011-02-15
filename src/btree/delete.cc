#include "btree/delete.hpp"

#include "btree/modify_oper.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    delete_result_t result;

    bool operate(const boost::shared_ptr<transactor_t>& txor, btree_value *old_value, large_buf_lock_t& old_large_buflock, btree_value **new_value, large_buf_lock_t& new_large_buflock) {
        if (old_value) {
            result = dr_deleted;
            *new_value = NULL;
            return true;
        } else {
            result = dr_not_found;
            return false;
        }
    }
};

delete_result_t btree_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp timestamp) {
    btree_delete_oper_t oper;
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, timestamp));
    return oper.result;
}
