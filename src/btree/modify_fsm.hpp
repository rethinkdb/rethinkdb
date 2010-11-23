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
        start_transaction,
        acquire_superblock,
        acquire_root,
        acquire_node,
        acquire_large_value,
        operating,
        acquire_sibling,
        update_complete,
        committing
    };

public:
    explicit btree_modify_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : btree_fsm_t(_key, store),
          state(start_transaction),
          sb_buf(NULL), buf(NULL), last_buf(NULL), sib_buf(NULL),
          node_id(NULL_BLOCK_ID), last_node_id(NULL_BLOCK_ID),
          sib_node_id(NULL_BLOCK_ID), operated(false),
          have_computed_new_value(false), new_value(NULL),
          update_needed(false), did_split(false), cas_already_set(false),
          dest_reached(false), key_found(false), old_large_buf(NULL)
        {}

    transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }

    /* btree_modify_fsm calls operate() when it finds the leaf node.
     * 'old_value' is the previous value or NULL if the key was not present
     * before. If operate() succeeds (returns true), 'new_value' should be
     * filled with a pointer to the new value or NULL if the key should be
     * deleted; *new_value should remain valid as long as the FSM is alive.
     */
    virtual transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) = 0;
    virtual void on_operate_completed() {} // TODO: Rename this.

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

    // When we reach the leaf node and it's time to call operate(), we store the result in
    // 'new_value' until we are ready to insert it.
    bool operated;
    bool have_computed_new_value; // XXX Rename this -- name is too confusing next to "operated".

    btree_value *new_value;

    bool expired;

protected:
    bool update_needed;
    bool did_split; /* XXX when an assert on this fails it means EPSILON is wrong */
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
