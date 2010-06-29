
#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

template <class config_t>
class btree_set_fsm : public btree_fsm<config_t>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<typename config_t::alloc_t>, btree_set_fsm<config_t> >
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
        acquire_superblock,
        acquire_root,
        insert_root,
        insert_root_on_split,
        acquire_node,
        update_complete
    };

public:
    explicit btree_set_fsm(cache_t *cache)
        : btree_fsm_t(cache, btree_fsm_t::btree_set_fsm),
          state(uninitialized), buf(NULL), last_buf(NULL), node_id(cache_t::null_block_id),
          last_node_id(cache_t::null_block_id), loading_superblock(false),
          node_dirty(false), last_node_dirty(false)
        {}

    void init_update(int _key, int _value);
    virtual transition_result_t do_transition(event_t *event);

    virtual bool is_finished() { return state == update_complete; }

private:
    using btree_fsm<config_t>::transaction;
    using btree_fsm<config_t>::cache;

    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    transition_result_t do_insert_root(event_t *event);
    transition_result_t do_insert_root_on_split(event_t *event);
    transition_result_t do_acquire_node(event_t *event);

private:
    int set_root_id(block_id_t root_id, event_t *event);
    void split_node(node_t *node, buf_t **rnode, block_id_t *rnode_id, int *median);
    
private:
    // Some relevant state information
    state_t state;
    int key;
    int value;

    buf_t *buf;
    buf_t *last_buf; /* This is always an internal_node. */
    block_id_t node_id, last_node_id; /* XXX These should be removed. */
    bool loading_superblock, node_dirty, last_node_dirty;
};

#include "btree/set_fsm_impl.hpp"

#endif // __BTREE_SET_FSM_HPP__

