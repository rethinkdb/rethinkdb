#ifndef __BTREE_MODIFY_FSM_HPP__
#define __BTREE_MODIFY_FSM_HPP__

#include "btree/fsm.hpp"
#include "btree/node.hpp"

/* Stats */
extern perfmon_counter_t pm_btree_depth;

class btree_modify_fsm_t : public btree_fsm_t {
public:
    typedef btree_fsm_t::transition_result_t transition_result_t;
    using btree_fsm_t::key;
    
public:

    enum state_t {
        go_to_cache_core,
        start_transaction,
        acquire_superblock,
        acquire_root,
        acquire_node,
        acquire_large_value,
        acquire_sibling,
        update_complete,
        committing,
        call_callback_and_delete_self
    };

public:
    explicit btree_modify_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : btree_fsm_t(_key, store),
          state(go_to_cache_core),
          sb_buf(NULL), buf(NULL), last_buf(NULL), sib_buf(NULL),
          node_id(NULL_BLOCK_ID), last_node_id(NULL_BLOCK_ID),
          sib_node_id(NULL_BLOCK_ID), in_operate_call(false), operated(false),
          have_computed_new_value(false), new_value(NULL),
          update_needed(false), did_split(false), cas_already_set(false),
          dest_reached(false), key_found(false), old_large_buf(NULL)
    {
    }

    void do_transition(event_t *event);

    /* btree_modify_fsm calls operate() when it finds the leaf node.
     * 'old_value' is the previous value or NULL if the key was not present
     * before. 'old_large_buf' is the large buf for the old value, if the old
     * value is a large value. When the subclass is done, it should call
     * have_finished_operating() or have_failed_operating(). If it calls
     * have_finished_operating(), the buffer it passes should remain valid
     * until the btree_modify_fsm is destroyed.
     */
    virtual void operate(btree_value *old_value, large_buf_t *old_large_buf) = 0;
    void have_finished_operating(btree_value *new_value);
    void have_failed_operating();
    
    /* btree_modify_fsm calls call_callback_and_delete() after it has
     * returned to the core on which it originated. Subclasses use
     * call_callback_and_delete() to report the results of the operation
     * to the originator of the request. call_callback_and_delete() must
     * "delete this;" when it is done. */
    virtual void call_callback_and_delete() = 0;

public:
    using btree_fsm_t::transaction;
    using btree_fsm_t::cache;

    transition_result_t do_start_transaction(event_t *event);
    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    void insert_root(block_id_t root_id);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_large_value(event_t *event);
    transition_result_t do_acquire_sibling(event_t *event);
    bool do_check_for_split(const node_t **node);
    void split_node(buf_t *node, buf_t **rnode, block_id_t *rnode_id, btree_key *median);

    // Some relevant state information
    state_t state;

    buf_t *sb_buf, *buf, *last_buf, *sib_buf;
    block_id_t node_id, last_node_id, sib_node_id; // TODO(NNW): Bufs may suffice for these.
    
    bool in_operate_call;   // Set to true before we call operate() and false when it returns
    bool operate_is_done;   // Set to true by have_finished_operating() and have_failed_operating()
    
    // When we reach the leaf node and it's time to call operate(), we store the result in
    // 'new_value' until we are ready to insert it.
    bool operated;
    bool have_computed_new_value; // XXX Rename this -- name is too confusing next to "operated".

    btree_value *new_value;

    bool expired;

private:
    bool update_needed;   // true if have_finished_operating(), false if have_failed_operating()
    bool did_split; /* XXX when an assert on this fails it means EPSILON is wrong */

protected:
    bool cas_already_set; // In case a sub-class needs to set the CAS itself.

private:
    bool dest_reached;
    bool key_found;

    union {
        byte old_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value old_value;
    };

    large_buf_t *old_large_buf;
    //large_buf_t *new_large_value;
};

// TODO: Figure out includes.
#include "conn_fsm.hpp"

#endif // __BTREE_MODIFY_FSM_HPP__
