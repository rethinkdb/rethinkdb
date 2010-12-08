#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_delete_expired_fsm_t :
    public btree_modify_fsm_t
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_delete_expired_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : btree_modify_fsm_t(_key, store)
    {
        do_transition(NULL);
    }

    void operate(btree_value *old_value, large_buf_t *old_large_buf) {
        /* Don't do anything. The modify_fsm will take advantage of the fact that we got to
        the leaf in write mode to automatically delete the expired key if necessary. */
        have_failed_operating();
    }

    void call_callback_and_delete() {
        delete this;
    }
};

void delete_expired(btree_key *key, btree_key_value_store_t *store) {
    new btree_delete_expired_fsm_t(key, store);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
