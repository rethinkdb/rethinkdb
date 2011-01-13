#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_delete_expired_fsm_t :
    public btree_modify_fsm_t
{
public:
    bool operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        /* Don't do anything. The modify_fsm will take advantage of the fact that we got to
        the leaf in write mode to automatically delete the expired key if necessary. */
        return false;
    }

    void call_callback_and_delete() {
        delete this;
    }
};

void co_btree_delete_expired(btree_key *key, btree_key_value_store_t *store) {
    btree_delete_expired_fsm_t *fsm = new btree_delete_expired_fsm_t();
    fsm->run(store, key);
}

void btree_delete_expired(btree_key *key, btree_key_value_store_t *store) {
    coro_t::spawn(co_btree_delete_expired, key, store);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
