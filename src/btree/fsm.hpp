#ifndef __BTREE_FSM_HPP__
#define __BTREE_FSM_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"
#include "event.hpp"

class btree_fsm_t :
    public cpu_message_t,
    public block_available_callback_t,
    public large_buf_available_callback_t,
    public transaction_begin_callback_t,
    public transaction_commit_callback_t,
    public home_cpu_mixin_t
{
public:
    enum transition_result_t {
        transition_incomplete,
        transition_ok
    };

public:
    btree_fsm_t(btree_key *_key, btree_key_value_store_t *store)
        : transaction(NULL), noreply(false), store(store)
    {
        keycpy(&key, _key);
        slice = store->slice_for_key(&key);
        cache = &slice->cache;
    }
    virtual ~btree_fsm_t() {}

    /* TODO: This function will be called many times per each
     * operation (once for blocks that are kept in memory, and once
     * for each AIO request). In addition, for a btree of btrees, it
     * will get called once for the inner btree per each block. We
     * might want to consider hardcoding a switch statement instead of
     * using a virtual function, and testing the performance
     * difference. */

    virtual transition_result_t do_transition(event_t *event) = 0;
    
    void done();
    
    void on_cpu_switch();
    void on_block_available(buf_t *buf);
    void on_large_buf_available(large_buf_t *large_buf);
    void on_txn_begin(transaction_t *txn);
    void on_txn_commit(transaction_t *txn);
    
    void step(); // XXX Rename this.

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
