#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_delete_expired_fsm_t : public btree_modify_fsm_t,
                                   public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_expired_fsm_t> {
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_delete_expired_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : btree_modify_fsm_t(_key, store) {}

    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        // modify_fsm will take care of everything.
        this->update_needed = false;
        return btree_fsm_t::transition_ok;
    }
};

void delete_expired(btree_key *key, btree_key_value_store_t *store) {
    btree_delete_expired_fsm_t *fsm = new btree_delete_expired_fsm_t(key, store);
    fsm->run(NULL);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
