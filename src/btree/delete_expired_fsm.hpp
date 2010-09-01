#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_delete_expired_fsm : public btree_modify_fsm<config_t>,
                                 public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm<config_t> > {
    typedef typename config_t::large_buf_t large_buf_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_delete_expired_fsm(btree_key *_key)
        : btree_modify_fsm<config_t>(_key) {}

    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        // modify_fsm will take care of everything.
        this->update_needed = false;
        return btree_fsm_t::transition_ok;
    }
};

template <class config_t>
void delete_expired(btree_key *key) {
    worker_t *worker = get_cpu_context()->worker;
    btree_delete_expired_fsm<config_t> *fsm = new btree_delete_expired_fsm<config_t>(key);
    fsm->request = NULL;
    fsm->return_cpu = worker->workerid;
    worker->event_queue->message_hub.store_message(worker->workerid, fsm);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
