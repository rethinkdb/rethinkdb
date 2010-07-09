
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
    typedef typename config_t::cache_t cache_t;
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
        insert_root_on_collapse,
        committing,
    };

    enum op_result_t {
        btree_found,
        btree_not_found
    };

public:
    explicit btree_delete_fsm(cache_t *cache)
        : btree_fsm_t(cache, btree_fsm_t::btree_delete_fsm),
          state(uninitialized), key((btree_key*)key_memory), buf(NULL), last_buf(NULL), sb_buf(NULL),  sib_buf(NULL), node_id(cache_t::null_block_id)
        {}

    void init_delete(btree_key *_key);
    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }

public:
    op_result_t op_result;

private:
    using btree_fsm<config_t>::cache;
    using btree_fsm<config_t>::transaction;

    transition_result_t do_start_transaction(event_t *event);
    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_sibling(event_t *event);
    transition_result_t do_insert_root_on_collapse(event_t *event);

    int set_root_id(block_id_t root_id, event_t *event);

private:
    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    // Some relevant state information
    state_t state;
    btree_key * const key;
    buf_t *buf, *last_buf, *sb_buf, *sib_buf;
    block_id_t node_id, last_node_id, sib_node_id;

};

#include "btree/delete_fsm_impl.hpp"

#endif // __BTREE_DELETE_FSM_HPP__

