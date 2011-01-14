#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_delete_expired_fsm_t :
    public btree_modify_fsm_t
{
public:
    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        /* Don't do anything. The modify_fsm will take advantage of the fact that we got to
        the leaf in write mode to automatically delete the expired key if necessary. */
        return false;
    }

    void call_callback_and_delete() {
        delete this;
    }
};

// This function is called when doing a btree_get() and finding an expired key.
// However, sometimes the get coroutine finishes before the delete_expired
// coroutine has a chance to start and copy the key from the get coroutine's
// stack. So for now we'll make a copy of the key and then free it when
// delete_expired is done.

void co_btree_delete_expired(btree_key *key_copy, btree_key_value_store_t *store) {
    btree_delete_expired_fsm_t *fsm = new btree_delete_expired_fsm_t();
    fsm->run(store, key_copy);
    free(key_copy);
}

void btree_delete_expired(btree_key *key, btree_key_value_store_t *store) {
    btree_key *key_copy = (btree_key *) malloc(key->size + sizeof(btree_key));
    keycpy(key_copy, key);
    coro_t::spawn(co_btree_delete_expired, key_copy, store);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
