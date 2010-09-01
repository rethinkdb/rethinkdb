#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_get_fsm : public btree_fsm<config_t>, public large_value_read_callback, public large_value_completed_callback,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_get_fsm<config_t> > {
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
    typedef typename cache_t::buf_t buf_t;
    typedef typename config_t::large_buf_t large_buf_t;
    typedef typename config_t::write_large_value_msg_t write_large_value_msg_t;

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
    explicit btree_get_fsm(btree_key *_key, request_callback_t *req)
        : btree_fsm_t(_key),
          large_value(NULL), req(req), state(acquire_superblock), last_buf(NULL), buf(NULL), node_id(NULL_BLOCK_ID)
        {}

    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() { return state == lookup_complete; }

    void on_large_value_read();
    void on_large_value_completed(bool _success);

public:
    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

    large_buf_t *large_value;

private:
    using btree_fsm<config_t>::cache;
    using btree_fsm<config_t>::transaction;

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

#include "btree/get_fsm.tcc"

#endif // __BTREE_GET_FSM_HPP__
