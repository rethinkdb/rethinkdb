
#ifndef __BTREE_MODIFY_FSM_HPP__
#define __BTREE_MODIFY_FSM_HPP__

#include "btree/fsm.hpp"

template <class config_t>
class btree_modify_fsm : public btree_fsm<config_t>
{
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
    typedef typename cache_t::buf_t buf_t;
    
    using btree_fsm_t::key;
    
public:

    enum state_t {
        start_transaction,
        acquire_superblock,
        acquire_root,
        acquire_node,
        insert_root,
        delete_complete,
        acquire_sibling,
        insert_root_on_collapse,
        insert_root_on_split,
        update_complete,
        committing
    };

    enum op_result_t {
        btree_incomplete,
        btree_found,
        btree_not_found
    };

public:
    explicit btree_modify_fsm(btree_key *_key)
        : btree_fsm_t(_key),
          state(start_transaction),
          sb_buf(NULL), buf(NULL), last_buf(NULL),
          node_id(cache_t::null_block_id), last_node_id(cache_t::null_block_id),
          have_computed_new_value(false), new_value(NULL),
          set_was_successful(false),
          op_result(btree_incomplete)
        {}

    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }
    
    /*
    btree_modify_fsm calls operate() when it finds the leaf node. 'old_value' is the previous value
    or NULL if the key was not present before. 'new_value' should be filled with a pointer to the
    new value; *new_value should remain valid as long as the FSM is alive.
    */
    virtual bool operate(btree_value *old_value, btree_value **new_value) = 0;
        
public:
    using btree_fsm<config_t>::transaction;
    using btree_fsm<config_t>::cache;

    virtual transition_result_t do_start_transaction(event_t *event);
    virtual transition_result_t do_acquire_superblock(event_t *event);
    virtual transition_result_t do_acquire_root(event_t *event);
    virtual transition_result_t do_insert_root(event_t *event);
    virtual transition_result_t do_insert_root_on_split(event_t *event);
    virtual transition_result_t do_insert_root_on_collapse(event_t *event);
    virtual transition_result_t do_acquire_node(event_t *event);
    virtual transition_result_t do_check_for_split(node_t *node);
    virtual transition_result_t do_acquire_sibling(event_t *event);
    
#ifndef NDEBUG
    // Print debugging information designed to resolve deadlocks
    void deadlock_debug();
#endif
    
public:
    int set_root_id(block_id_t root_id, event_t *event);
    void split_node(buf_t *node, buf_t **rnode, block_id_t *rnode_id, btree_key *median);
    
    // Some relevant state information
    state_t state;
    
    buf_t *sb_buf, *buf, *last_buf;
    block_id_t node_id, last_node_id; // TODO(NNW): Bufs may suffice for these.
    
    // When we reach the leaf node and it's time to call operate(), we store the result in
    // 'new_value' until we are ready to insert it.
    bool have_computed_new_value;
    btree_value *new_value;
    
public:
    bool set_was_successful;
    op_result_t op_result;

public: // from delete_fsm:
    buf_t *sib_buf;
    block_id_t sib_node_id;

};

#include "btree/modify_fsm.tcc"

#endif // __BTREE_MODIFY_FSM_HPP__

