
#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_get_fsm : public btree_fsm<config_t>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_get_fsm<config_t> >
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
        acquire_superblock,
        acquire_root,
        acquire_node,
        lookup_complete
    };

    enum op_result_t {
        btree_found,
        btree_not_found
    };

public:
    explicit btree_get_fsm(cache_t *cache)
        : btree_fsm_t(cache, btree_fsm_t::btree_get_fsm),
          key((btree_key*)key_memory), value((btree_value*)value_memory), state(uninitialized), last_buf(NULL), buf(NULL), node_id(cache_t::null_block_id)
        {}

    void init_lookup(btree_key *_key);
    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() { return state == lookup_complete; }

public:
    op_result_t op_result;
    btree_key * const key;
    btree_value * const value;

private:
    using btree_fsm<config_t>::cache;
    using btree_fsm<config_t>::transaction;

    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);

#ifndef NDEBUG
    // Print debugging information designed to resolve deadlocks
    void deadlock_debug();
#endif
    
private:
    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    char value_memory[MAX_IN_NODE_VALUE_SIZE+sizeof(btree_value)];
    // Some relevant state information
    state_t state;
    buf_t *last_buf, *buf;
    block_id_t node_id;
};

#include "btree/get_fsm.tcc"

#endif // __BTREE_GET_FSM_HPP__

