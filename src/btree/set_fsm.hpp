
#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

typedef enum {
    btree_set_type_set,
    btree_set_type_add,
    btree_set_type_replace,
    btree_set_type_incr,
    btree_set_type_decr
}btree_set_type;

template <class config_t>
class btree_set_fsm : public btree_fsm<config_t>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_set_fsm<config_t> >
{
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
    typedef typename cache_t::buf_t buf_t;

public:
    enum state_t {
        uninitialized,
        start_transaction,
        acquire_superblock,
        acquire_root,
        insert_root,
        insert_root_on_split,
        acquire_node,
        update_complete,
        committing
    };

public:
    explicit btree_set_fsm(cache_t *cache)
        : btree_fsm_t(cache, btree_fsm_t::btree_set_fsm),
          state(uninitialized), key((btree_key*)key_memory), value((btree_value*)value_memory), sb_buf(NULL), buf(NULL), last_buf(NULL),
          node_id(cache_t::null_block_id), last_node_id(cache_t::null_block_id), set_was_successful(false)
        {}

    void init_update(btree_key *_key, byte *data, unsigned int length, btree_set_type set_type);
    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }
    
    btree_key* get_key();
    btree_value* get_value();
    btree_set_type get_set_type();
    
private:
    using btree_fsm<config_t>::transaction;
    using btree_fsm<config_t>::cache;

    transition_result_t do_start_transaction(event_t *event);
    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_insert_root(event_t *event);
    transition_result_t do_insert_root_on_split(event_t *event);
    transition_result_t do_acquire_node(event_t *event);

#ifndef NDEBUG
    // Print debugging information designed to resolve deadlocks
    void deadlock_debug();
#endif
    
private:
    int set_root_id(block_id_t root_id, event_t *event);
    void split_node(buf_t *node, buf_t **rnode, block_id_t *rnode_id, btree_key *median);
    
private:
    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    char value_memory[MAX_IN_NODE_VALUE_SIZE+sizeof(btree_value)];
    // Some relevant state information
    state_t state;
public:
    btree_key * const key;
    btree_value * const value;

private:
    buf_t *sb_buf, *buf, *last_buf;
    block_id_t node_id, last_node_id; // TODO(NNW): Bufs may suffice for these.
    
    btree_set_type set_type; // choice of set, add or replace with a different behaviour for each.
public:
    bool set_was_successful;
};



#include "btree/set_fsm.tcc"

#endif // __BTREE_SET_FSM_HPP__

