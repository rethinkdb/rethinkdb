#ifndef __BTREE_DELETE_HPP__
#define __BTREE_DELETE_HPP__

#include "btree/modify_oper.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

class btree_delete_oper_t : public btree_modify_oper_t {
    store_t::delete_callback_t *callback;
public:

    explicit btree_delete_oper_t(store_t::delete_callback_t *cb)
        : btree_modify_oper_t(), callback(cb)
    { }

    bool exists;

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        exists = bool(old_value);
        if (exists) {
            *new_value = NULL;
            *new_large_buf = NULL;
            return true;
        } else {
            return false;
        }
    }

    void call_callback() {
        if (exists) callback->deleted();
        else callback->not_found();
    }
};

void co_btree_delete(const btree_key *key, btree_key_value_store_t *store, store_t::delete_callback_t *cb) {
    btree_delete_oper_t *oper = new btree_delete_oper_t(cb);
    run_btree_modify_oper(oper, store, key);
}

void btree_delete(const btree_key *key, btree_key_value_store_t *store, store_t::delete_callback_t *cb) {
    coro_t::spawn(co_btree_delete, key, store, cb);
}

#endif // __BTREE_DELETE_HPP__
