
#ifndef __BTREE_DELETE_FSM_HPP__
#define __BTREE_DELETE_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_delete_fsm : public btree_modify_fsm<config_t>,
                         public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm<config_t> >
{
public:
    typedef typename btree_modify_fsm<config_t>::cache_t cache_t;
    typedef typename btree_modify_fsm<config_t>::transition_result_t transition_result_t;
    typedef typename btree_modify_fsm<config_t>::buf_t buf_t;
    typedef typename btree_modify_fsm<config_t>::btree_fsm_t btree_fsm_t;
    typedef typename btree_modify_fsm<config_t>::node_t node_t;

    using btree_modify_fsm<config_t>::state;
    using btree_modify_fsm<config_t>::sb_buf;
    using btree_modify_fsm<config_t>::buf;
    using btree_modify_fsm<config_t>::last_buf;
    using btree_modify_fsm<config_t>::node_id;
    using btree_modify_fsm<config_t>::last_node_id;
    using btree_modify_fsm<config_t>::have_computed_new_value;
    using btree_modify_fsm<config_t>::new_value;
    using btree_modify_fsm<config_t>::set_was_successful;
    using btree_modify_fsm<config_t>::sib_buf;
    using btree_modify_fsm<config_t>::sib_node_id;
    using btree_modify_fsm<config_t>::op_result;

    // the enum for deletes in modify_fsm
    using btree_modify_fsm<config_t>::btree_incomplete;
    using btree_modify_fsm<config_t>::btree_found;
    using btree_modify_fsm<config_t>::btree_not_found;

    // the enum
    using btree_modify_fsm<config_t>::insert_root;
    using btree_modify_fsm<config_t>::start_transaction;
    using btree_modify_fsm<config_t>::acquire_superblock;
    using btree_modify_fsm<config_t>::acquire_root;
    using btree_modify_fsm<config_t>::acquire_node;
    using btree_modify_fsm<config_t>::insert_root;
    using btree_modify_fsm<config_t>::delete_complete;
    using btree_modify_fsm<config_t>::acquire_sibling;
    using btree_modify_fsm<config_t>::insert_root_on_collapse;
    using btree_modify_fsm<config_t>::insert_root_on_split;
    using btree_modify_fsm<config_t>::update_complete;
    using btree_modify_fsm<config_t>::committing;

    // from btree_fsm
    using btree_modify_fsm<config_t>::transaction;
    using btree_modify_fsm<config_t>::cache;
    using btree_modify_fsm<config_t>::key;

    // methods
    using btree_modify_fsm<config_t>::do_start_transaction;
    using btree_modify_fsm<config_t>::do_acquire_superblock;
    using btree_modify_fsm<config_t>::do_insert_root;
    using btree_modify_fsm<config_t>::do_acquire_root;
    using btree_modify_fsm<config_t>::do_acquire_node;
    using btree_modify_fsm<config_t>::is_finished;
    using btree_modify_fsm<config_t>::do_insert_root_on_collapse;
    using btree_modify_fsm<config_t>::do_acquire_sibling;
public:
    explicit btree_delete_fsm(btree_key *_key)
        : btree_modify_fsm<config_t>(_key)          
        {}
//    virtual transition_result_t do_transition(event_t *event);
    bool operate(btree_value *old_value, btree_value **new_value) {
        // If the key didn't exist before, we fail
        if (!old_value) return false;
        return true;
    }
    
public:
    // methods that delete_fsm has but modify_fsm doesn't.
    // These should be empty virtual functions in modify_fsm.
    void split_internal_node(buf_t *buf, buf_t **rbuf, block_id_t *rnode_id, btree_key *median);
    transition_result_t do_insert_root_on_split(event_t *event);
    
    // methods in modify_fsm that delete_fsm overrides
    transition_result_t do_insert_root(event_t *event);

};

#include "btree/delete_fsm.tcc"

#endif // __BTREE_DELETE_FSM_HPP__

