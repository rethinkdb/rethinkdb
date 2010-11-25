#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "event.hpp"
#include "btree/fsm.hpp"

class btree_get_fsm_t :
    public btree_fsm_t,
    public store_t::get_callback_t::done_callback_t
{
public:
    typedef btree_fsm_t::transition_result_t transition_result_t;
    using btree_fsm_t::key;

public:
    enum state_t {
        go_to_cache_core,
        acquire_superblock,
        acquire_root,
        acquire_node,
        
        acquire_large_value,
        deliver_large_value,
        write_large_value,
        return_after_deliver_large_value,
        free_large_value,
        
        deliver_small_value,
        deliver_not_found_notification,
        
        delete_self
    };

public:
    explicit btree_get_fsm_t(btree_key *_key, btree_key_value_store_t *store, store_t::get_callback_t *cb)
        : btree_fsm_t(_key, store),
          large_value(NULL), callback(cb), state(go_to_cache_core), last_buf(NULL), buf(NULL), node_id(NULL_BLOCK_ID)
    {
        pm_cmd_get.begin(&start_time);
        do_transition(NULL);
    }

    ~btree_get_fsm_t() {
        pm_cmd_get.end(&start_time);
    }
    
    virtual void do_transition(event_t *event);
    void have_copied_value(); /* Called by the request handler when it's done with the value we gave it */
    bool in_callback_value_call, value_was_copied;
    
public:
    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    large_buf_t *large_value;
    buffer_group_t value_buffers;

private:
    using btree_fsm_t::cache;
    using btree_fsm_t::transaction;

    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_large_value(event_t *event);
    transition_result_t do_deliver_large_value(event_t *event);
    transition_result_t do_write_large_value(event_t *event);
    transition_result_t do_return_after_deliver_large_value(event_t *event);
    transition_result_t do_free_large_value(event_t *event);
    transition_result_t do_deliver_small_value(event_t *event);
    transition_result_t do_deliver_not_found_notification(event_t *event);

private:
    ticks_t start_time;
    store_t::get_callback_t *callback;
    
    // Some relevant state information
public: // XXX
    state_t state;
    buf_t *last_buf, *buf;
    block_id_t node_id;
};

#endif // __BTREE_GET_FSM_HPP__
