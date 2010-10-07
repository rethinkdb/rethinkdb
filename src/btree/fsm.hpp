#ifndef __BTREE_FSM_HPP__
#define __BTREE_FSM_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"
#include "event.hpp"

struct btree_fsm_callback_t {
    virtual void on_btree_fsm_complete() = 0;
};

class btree_fsm_t :
    public cpu_message_t,
    //public large_value_filled_callback, // XXX This should be a separate callback.
    //public large_value_completed_callback, // XXX Rename.
    public block_available_callback_t,
    public large_buf_available_callback_t,
    public transaction_begin_callback_t,
    public transaction_commit_callback_t,
    public home_cpu_mixin_t
{
public:
    enum transition_result_t {
        transition_incomplete,
        transition_ok,
        transition_complete
    };

    // TODO: Use status codes everywhere.
    enum status_code_t {
        S_UNKNOWN = 0,
        S_SUCCESS,
        S_KEY_NOT_FOUND,
        S_NOT_STORED,
        S_EXISTS,
        S_NOT_FOUND,
        S_DELETED,
        S_NOT_NUMERIC
    };


public:
    btree_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : transaction(NULL), noreply(false), status_code(S_UNKNOWN), store(store)
    {
        keycpy(&key, _key);
        slice = store->slice_for_key(&key);
        cache = &slice->cache;
    }
    virtual ~btree_fsm_t() {}
    
    bool run(btree_fsm_callback_t *cb) {
        assert_cpu();
        callback = cb;
        if (continue_on_cpu(cache->home_cpu, this)) call_later_on_this_cpu(this);
        return false;
    }

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
    
    void done();
    
    // Return true if the state machine is in a completed state
    virtual bool is_finished() = 0;

    virtual void on_block_available(buf_t *buf);
    virtual void on_large_buf_available(large_buf_t *large_buf);
    virtual void on_txn_begin(transaction_t *txn);
    virtual void on_txn_commit(transaction_t *txn);
    virtual void step(); // XXX Rename this.
    
    void on_cpu_switch();

public:
    union {
        char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
        btree_key key;
    };

    transaction_t *transaction;
    cache_t *cache;
    btree_fsm_callback_t *callback;
    bool noreply;

    status_code_t status_code;
    
    btree_key_value_store_t *store;
    btree_slice_t *slice;
};

#endif // __BTREE_FSM_HPP__
