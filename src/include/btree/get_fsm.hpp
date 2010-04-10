
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
    typedef typename config_t::fsm_t fsm_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
public:
    enum state_t {
        uninitialized,
        lookup_acquiring_superblock,
        lookup_acquiring_root,
        lookup_acquiring_node
    };

    enum op_result_t {
        btree_found,
        btree_not_found
    };

public:
    btree_get_fsm(cache_t *_cache, fsm_t *_netfsm)
        : btree_fsm_t(_cache, _netfsm), state(uninitialized),
          node(NULL), node_id(cache_t::null_block_id)
        {}

    void init_lookup(int _key);
    virtual transition_result_t do_transition(event_t *event);

public:
    op_result_t op_result;
    int value;

private:
    transition_result_t do_lookup_acquiring_superblock();
    transition_result_t do_lookup_acquiring_root();
    transition_result_t do_lookup_acquiring_node();

private:
    // Some relevant state information
    state_t state;
    node_t *node;
    int key;

    block_id_t node_id;
};

#include "btree/get_fsm_impl.hpp"

#endif // __BTREE_GET_FSM_HPP__

