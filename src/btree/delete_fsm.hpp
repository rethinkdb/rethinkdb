
#ifndef __BTREE_DELETE_FSM_HPP__
#define __BTREE_DELETE_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_delete_fsm : public btree_modify_fsm<config_t>,
                         public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm<config_t> >
{
public:
    explicit btree_delete_fsm(btree_key *_key)
        : btree_modify_fsm<config_t>(_key)          
        {}
    bool operate(btree_value *old_value, btree_value **new_value) {
    	// TODO: Joe, add stats here
        // If the key didn't exist before, we fail
        if (!old_value) {
            this->status_code = btree_fsm<config_t>::S_NOT_FOUND;
            return false;
        }
        *new_value = NULL;
        this->status_code = btree_fsm<config_t>::S_DELETED; // XXX Should this just be S_SUCCESS?
        return true;
    }
};

#endif // __BTREE_DELETE_FSM_HPP__
