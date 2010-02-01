
#ifndef __BTREE_FSM_HPP__
#define __BTREE_FSM_HPP__

#include "corefwd.hpp"

template <class config_t>
class btree_fsm {
public:
    typedef typename config_t::btree_t btree_t;
    typedef typename config_t::serializer_t::block_id_t block_id_t;
    
public:
    enum state_t {
        lookup_waiting_for_superblock,
        lookup_waiting_for_root,
        lookup_waiting_for_node
    };

public:
    btree_fsm(btree_t *_btree, state_t _state)
        : state(_state), btree(_btree)
        {}
    
public:
    state_t state;
    btree_t *btree;

    // Lookup mode
    block_id_t node_id;
    int key, *value;
};

#endif // __BTREE_FSM_HPP__

