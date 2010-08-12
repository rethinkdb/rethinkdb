#ifndef __BTREE_DELETE_EXPIRED_FSM_HPP__
#define __BTREE_DELETE_EXPIRED_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_delete_expired_fsm : public btree_modify_fsm<config_t>,
                                 public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm<config_t> > {
public:
    explicit btree_delete_expired_fsm(btree_key *_key)
        : btree_modify_fsm<config_t>(_key) {}

    bool operate(btree_value *old_value, btree_value **new_value) {
        // modify_fsm will take care of everything.
        return false;
    }
};

template <class config_t>
void delete_expired(btree_key *key) {
    worker_t *worker = get_cpu_context()->worker;
    logf(DBG, "Deleting %*.*s\n", key->size, key->size, key->contents);
    btree_delete_expired_fsm<config_t> *fsm = new btree_delete_expired_fsm<config_t>(key);
    fsm->request = NULL;
    fsm->return_cpu = worker->workerid;
    worker->event_queue->message_hub.store_message(worker->workerid, fsm);
}

#endif // __BTREE_DELETE_EXPIRED_FSM_HPP__
