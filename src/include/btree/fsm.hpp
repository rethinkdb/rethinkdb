
#ifndef __FSM_BTREE_HPP__
#define __FSM_BTREE_HPP__

#include <assert.h>
#include "message_hub.hpp"
#include "buffer_cache/callbacks.hpp"

template <class config_t>
class btree_fsm : public cpu_message_t,
                  public block_available_callback<config_t>,
                  public transaction_begin_callback<config_t>,
                  public transaction_commit_callback<config_t>
{
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename cache_t::transaction_t transaction_t;
    typedef typename cache_t::buf_t buf_t;
    typedef void (*on_complete_t)(btree_fsm_t* btree_fsm);
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
        btree_delete_fsm,
    };

public:
    btree_fsm(cache_t *_cache, fsm_type_t _fsm_type)
        : cpu_message_t(cpu_message_t::mt_btree),
          fsm_type(_fsm_type), transaction(NULL), request(NULL), cache(_cache),
          on_complete(NULL)
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

    // Return true if the state machine is in a completed state
    virtual bool is_finished() = 0;

    virtual void on_block_available(buf_t *buf);
    virtual void on_txn_begin(transaction_t *txn);
    virtual void on_txn_commit(transaction_t *txn);

#ifndef NDEBUG
    virtual void deadlock_debug() {
        printf("unknown-fsm %p\n", this);
    }
#endif
    
protected:
    block_id_t get_root_id(void *superblock_buf);
    cache_t* get_cache() {
        return cache;
    }

public:
    fsm_type_t fsm_type;
    transaction_t *transaction;
    request_t *request;
    cache_t *cache;
    on_complete_t on_complete;
};

#include "btree/fsm_impl.hpp"

#endif // __FSM_BTREE_HPP__

