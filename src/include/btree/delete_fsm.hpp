
#ifndef __BTREE_DELETE_FSM_HPP__
#define __BTREE_DELETE_FSM_HPP__

#include "corefwd.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"

template <class config_t>
class btree_delete_fsm : public btree_fsm<config_t>,
                         public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_delete_fsm<config_t> >
{
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename node_t::leaf_node_t leaf_node_t;
    typedef typename node_t::internal_node_t internal_node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename cache_t::block_id_t block_id_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
    typedef typename cache_t::buf_t buf_t;

public:
    enum state_t {
        uninitialized,
        start_transaction,
        acquire_superblock,
        acquire_root,
        acquire_node,
        delete_complete,
        acquire_sibling,
        committing,
    };

    enum op_result_t {
        btree_found,
        btree_not_found
    };

public:
    explicit btree_delete_fsm(cache_t *cache)
        : btree_fsm_t(cache, btree_fsm_t::btree_get_fsm),
          state(uninitialized), buf(NULL), last_buf(NULL), node_id(cache_t::null_block_id)
        {}

    void init_delete(int _key);
    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }

public:
    op_result_t op_result;
    int value;
    int key;

private:
    using btree_fsm<config_t>::cache;
    using btree_fsm<config_t>::transaction;

    transition_result_t do_start_transaction(event_t *event);
    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_sibling(event_t *event);

private:
    // Some relevant state information
    state_t state;
    buf_t *buf;
    buf_t *last_buf;
    buf_t *sib_buf;
    block_id_t node_id, last_node_id, sib_node_id;
};

#include "btree/delete_fsm_impl.hpp"

#endif // __BTREE_DELETE_FSM_HPP__

