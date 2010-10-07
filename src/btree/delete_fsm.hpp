#ifndef __BTREE_DELETE_FSM_HPP__
#define __BTREE_DELETE_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/fsm.hpp"

class btree_delete_fsm_t : public btree_modify_fsm_t,
                           public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm_t> {
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_delete_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : btree_modify_fsm_t(_key, store)
        {}
    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
    	// TODO: Joe, add stats here
        // If the key didn't exist before, we fail
        if (!old_value) {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
        }
        *new_value = NULL;
        this->status_code = btree_fsm_t::S_DELETED; // XXX Should this just be S_SUCCESS?
        this->update_needed = true;
        return btree_fsm_t::transition_ok;
    }
};

#endif // __BTREE_DELETE_FSM_HPP__
