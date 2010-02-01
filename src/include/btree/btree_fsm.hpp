
#ifndef __BTREE_FSM_HPP__
#define __BTREE_FSM_HPP__

template <typename block_id_t>
class btree_fsm_t {
public:
    enum state_t {
        lookup_waiting_for_superblock,
        lookup_waiting_for_root,
        lookup_waiting_for_node
    };
    state_t state;

    // Lookup mode
    block_id_t node_id;
    int key, *value;
};

#endif // __BTREE_FSM_HPP__

