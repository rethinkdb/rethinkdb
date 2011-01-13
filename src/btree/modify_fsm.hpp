#ifndef __BTREE_MODIFY_FSM_HPP__
#define __BTREE_MODIFY_FSM_HPP__

#include "btree/node.hpp"
#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"

/* Stats */
extern perfmon_counter_t pm_btree_depth;

class btree_modify_fsm_t : public home_thread_mixin_t {
public:
    explicit btree_modify_fsm_t()
        : transaction(NULL), slice(NULL), buf(NULL),
          last_buf(NULL), cas_already_set(false)
    { }

    virtual ~btree_modify_fsm_t() { }

    /* btree_modify_fsm calls operate() when it finds the leaf node.
     * 'old_value' is the previous value or NULL if the key was not present
     * before. 'old_large_buf' is the large buf for the old value, if the old
     * value is a large value. When the subclass is done, it should call
     * have_finished_operating() or have_failed_operating(). If it calls
     * have_finished_operating(), the buffer it passes should remain valid
     * until the btree_modify_fsm is destroyed.
     */
    // TODO: Fix operate()'s documentation.
    virtual bool operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) = 0;

    /* btree_modify_fsm calls call_callback_and_delete() after it has
     * returned to the core on which it originated. Subclasses use
     * call_callback_and_delete() to report the results of the operation
     * to the originator of the request. call_callback_and_delete() must
     * "delete this;" when it is done. */
    virtual void call_callback_and_delete() = 0;

public:

    transaction_t *transaction;
    btree_slice_t *slice;

    void run(btree_key_value_store_t *store, btree_key *_key);

private:
    buf_t *get_root(buf_t **sb_buf, block_size_t block_size);
    void insert_root(block_id_t root_id, buf_t **sb_buf); // XXX: This should probably just get a buf_t *.
    void check_and_handle_split(const node_t **node, const btree_key *key, buf_t **sb_buf, btree_value *new_value, block_size_t block_size);
    void check_and_handle_underfull(const node_t **node, const btree_key *key, buf_t **sb_buf, block_size_t block_size);

    void split_node(buf_t *node, buf_t **rnode, block_id_t *rnode_id, btree_key *median, block_size_t block_size);

    void call_replicants(btree_key *key, btree_value *new_value, large_buf_t *new_large_buf, repli_timestamp new_value_timestamp);

    buf_t *buf, *last_buf;

    bool update_needed;   // Return value of operate().

protected:
    bool cas_already_set; // In case a sub-class needs to set the CAS itself.

    virtual void actually_acquire_large_value(large_buf_t *lb, const large_buf_ref& lbref);

private:

    // Replication-related stuff
    void have_copied_value();   // Called by replicants when they are done with the value we gave em
    bool in_value_call;
    int replicants_awaited;
};

#endif // __BTREE_MODIFY_FSM_HPP__
