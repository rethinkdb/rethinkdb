
#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"

template <class config_t>
class btree_get_fsm {
public:
    typedef typename config_t::node_t node_t;
    typedef typename node_t::leaf_node_t leaf_node_t;
    typedef typename node_t::internal_node_t internal_node_t;
    typedef typename config_t::fsm_t fsm_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    
public:
    enum state_t {
        uninitialized,
        lookup_acquiring_superblock,
        lookup_acquiring_root,
        lookup_acquiring_node
    };

    enum result_t {
        btree_transition_incomplete,
        btree_transition_ok,
        btree_get_fsm_complete
    };

    enum op_result_t {
        btree_found,
        btree_not_found
    };

public:
    btree_get_fsm(cache_t *_cache, fsm_t *_netfsm)
        : state(uninitialized), cache(_cache), netfsm(_netfsm)
        {}

    void init_lookup(int _key);
    result_t do_transition(event_t *event);

public:
    op_result_t op_result;
    int value;

private:
    result_t do_lookup_acquiring_superblock();
    result_t do_lookup_acquiring_root();
    result_t do_lookup_acquiring_node();

private:
    int get_root_id(block_id_t *root_id);
    
private:
    state_t state;
    cache_t *cache;
    fsm_t *netfsm;

    // Lookup mode
    block_id_t node_id;
    node_t *node;
    int key;
};

#include "btree/get_fsm_impl.hpp"

#endif // __BTREE_GET_FSM_HPP__

