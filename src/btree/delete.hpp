#ifndef __BTREE_DELETE_HPP__
#define __BTREE_DELETE_HPP__

#include "btree/modify_oper.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

struct btree_delete_oper_t : public btree_modify_oper_t {

    store_t::delete_result_t result;

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        if (old_value) {
            result = store_t::dr_deleted;
            *new_value = NULL;
            *new_large_buf = NULL;
            return true;
        } else {
            result = store_t::dr_not_found;
            return false;
        }
    }
};

store_t::delete_result_t btree_delete(const btree_key *key, btree_slice_t *slice) {
    btree_delete_oper_t oper;
    run_btree_modify_oper(&oper, slice, key);
    return oper.result;
}

#endif // __BTREE_DELETE_HPP__
