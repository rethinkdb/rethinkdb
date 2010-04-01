
#ifndef __FSM_BTREE_HPP__
#define __FSM_BTREE_HPP__

template <class config_t>
class btree_fsm {

public:
    virtual ~btree_fsm() {}

public:
    enum transition_result_t {
        transition_incomplete,
        transition_ok,
        transition_complete
    };

public:
    /* TODO: This function will be called many times per each
     * operation (once for blocks that are kept in memory, and once
     * for each AIO request). In addition, for a btree of btrees, it
     * will get called once for the inner btree per each block. We
     * might want to consider hardcoding a switch statement instead of
     * using a virtual function, and testing the performance
     * difference. */
    virtual transition_result_t do_transition(event_t *event) = 0;
};

#endif // __FSM_BTREE_HPP__

