#ifndef __BTREE_FSM_HPP__
#define __BTREE_FSM_HPP__

#include "utils.hpp"
#include "message_hub.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "event.hpp"

class btree_fsm_t : public cpu_message_t,
                    //public large_value_filled_callback, // XXX This should be a separate callback.
                    //public large_value_completed_callback, // XXX Rename.
                    public block_available_callback_t,
                    public large_buf_available_callback_t,
                    public transaction_begin_callback_t,
                    public transaction_commit_callback_t
{
public:
    typedef void (*on_complete_t)(btree_fsm_t *btree_fsm);
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
    btree_fsm_t(btree_key *_key)
        : cpu_message_t(cpu_message_t::mt_btree),
          transaction(NULL),
          cache(NULL),   // Will be set when we arrive at the core we are operating on
          on_complete(NULL), noreply(false), status_code(S_UNKNOWN)
        {
        keycpy(&key, _key);
    }
    virtual ~btree_fsm_t() {}

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
    virtual void on_large_buf_available(large_buf_t *large_buf);
    virtual void on_txn_begin(transaction_t *txn);
    virtual void on_txn_commit(transaction_t *txn);
    virtual void step(); // XXX Rename this.

public:
    union {
        char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
        btree_key key;
    };

    transaction_t *transaction;
    cache_t *cache;
    on_complete_t on_complete;
    bool noreply;

    status_code_t status_code;
};

#endif // __BTREE_FSM_HPP__
