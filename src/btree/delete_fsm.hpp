#ifndef __BTREE_DELETE_FSM_HPP__
#define __BTREE_DELETE_FSM_HPP__

#include "event.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/fsm.hpp"

class btree_delete_fsm_t : public btree_modify_fsm_t
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
    store_t::delete_callback_t *callback;
public:
    explicit btree_delete_fsm_t(btree_key *_key, btree_key_value_store_t *store, store_t::delete_callback_t *cb)
        : btree_modify_fsm_t(_key, store), callback(cb)
        {}
    bool exists;
    void operate(btree_value *old_value, large_buf_t *old_large_buf) {
        exists = bool(old_value);
        if (exists) have_finished_operating(NULL);
        else have_failed_operating();
    }
    
    void call_callback_and_delete() {
        if (exists) callback->success();
        else callback->not_found();
        delete this;
    }
};

#endif // __BTREE_DELETE_FSM_HPP__
