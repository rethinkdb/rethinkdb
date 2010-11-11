#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

class btree_get_fsm_t : public btree_fsm_t,
                        public large_value_completed_callback
{
public:
    typedef btree_fsm_t::transition_result_t transition_result_t;
    using btree_fsm_t::key;

public:
    enum state_t {
        acquire_superblock,
        acquire_root,
        acquire_node,
        acquire_large_value,
        large_value_acquired, // XXX: Rename this state.
        large_value_writing,
        lookup_complete
    };

public:
    explicit btree_get_fsm_t(btree_key *_key, btree_key_value_store_t *store, request_callback_t *req)
        : btree_fsm_t(_key, store),
          large_value(NULL), req(req), state(acquire_superblock), last_buf(NULL), buf(NULL), node_id(NULL_BLOCK_ID), write_lv_msg(NULL)
    {
        pm_cmd_get++;
    }

    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() { return state == lookup_complete; }

    void on_large_value_completed(bool _success);

    void begin_lv_write();

public:
    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

    large_buf_t *large_value;

private:
    using btree_fsm_t::cache;
    using btree_fsm_t::transaction;

    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_large_value(event_t *event);
    transition_result_t do_large_value_acquired(event_t *event);

private:
    request_callback_t *req;
    // Some relevant state information
public: // XXX
    state_t state;
    buf_t *last_buf, *buf;
    block_id_t node_id;

    write_large_value_msg_t *write_lv_msg;
};

#endif // __BTREE_GET_FSM_HPP__
