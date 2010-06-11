
#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_get_fsm : public btree_fsm<config_t> {
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename node_t::leaf_node_t leaf_node_t;
    typedef typename node_t::internal_node_t internal_node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename cache_t::block_id_t block_id_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
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
    btree_get_fsm()
        : btree_fsm_t(btree_fsm_t::btree_get_fsm),
          state(uninitialized), node(NULL), node_id(cache_t::null_block_id)
        {}

    void init_lookup(int _key);
    virtual transition_result_t do_transition(event_t *event, event_queue_t *event_queue);

    virtual bool is_finished() { return state == lookup_complete; }

public:
    op_result_t op_result;
    int value;

private:
    transition_result_t do_acquire_superblock(event_t *event, event_queue_t *event_queue);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);

private:
    // Some relevant state information
    state_t state;
    node_t *node;
    int key;

    block_id_t node_id;
};

#include "btree/get_fsm_impl.hpp"

#endif // __BTREE_GET_FSM_HPP__

