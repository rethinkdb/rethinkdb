
#ifndef __FSM_BTREE_HPP__
#define __FSM_BTREE_HPP__

template <class config_t>
class btree_fsm {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::fsm_t fsm_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;

public:
    btree_fsm(cache_t *_cache, fsm_t *_netfsm)
        : cache(_cache), netfsm(_netfsm)
        {}
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

    // If event is null, we're initiating the first transition
    // (i.e. do_transition is getting called for the first time in
    // this fsm instance). After this, do_transition should only get
    // called on disk events (when a node has been read from disk).
    virtual transition_result_t do_transition(event_t *event) = 0;

protected:
    int get_root_id(block_id_t *root_id);

public:
    cache_t *cache;
    fsm_t *netfsm;
};

#include "btree/fsm_impl.hpp"

#endif // __FSM_BTREE_HPP__

