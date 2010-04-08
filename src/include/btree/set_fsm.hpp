
#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

template <class config_t>
class btree_set_fsm : public btree_fsm<config_t> {
public:
    enum state_t {
        uninitialized,
        
    };

public:
    btree_set_fsm(cache_t *_cache, fsm_t *_netfsm)
        : btree_fsm_t(_cache, _netfsm), state(uninitialized)
        {}

    void init_insert(int _key, int _value);
    virtual transition_result_t do_transition(event_t *event);

private:
    // Some relevant state information
    state_t state;
    int key;
    int value;
};

#endif // __BTREE_SET_FSM_HPP__

