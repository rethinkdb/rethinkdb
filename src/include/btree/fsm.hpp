
#ifndef __FSM_BTREE_HPP__
#define __FSM_BTREE_HPP__

#include <assert.h>

template <class config_t>
class btree_fsm {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename cache_t::block_id_t block_id_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename cache_t::transaction_t transaction_t;

public:
    enum transition_result_t {
        transition_incomplete,
        transition_ok,
        transition_complete
    };

    // It's annoying that we have to do this, and it would probably be
    // nicer to do it with a visitor pattern (oh C++, give me
    // multimethods), but meh.
    enum fsm_type_t {
        btree_mock_fsm,
        btree_get_fsm,
        btree_set_fsm,
    };

public:
    btree_fsm(cache_t *_cache, conn_fsm_t *_netfsm, fsm_type_t _fsm_type)
        : cache(_cache), netfsm(_netfsm), fsm_type(_fsm_type), transaction(NULL)
        {}
    virtual ~btree_fsm() {}

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
    block_id_t get_root_id(void *superblock_buf);

public:
    cache_t *cache;
    conn_fsm_t *netfsm;
    fsm_type_t fsm_type;
    transaction_t *transaction;
};

#include "btree/fsm_impl.hpp"

#endif // __FSM_BTREE_HPP__

