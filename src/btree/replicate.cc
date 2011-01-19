#include "btree/replicate.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "btree/key_value_store.hpp"
#include "btree/slice.hpp"

struct slice_walker_t;

void walk_slice(btree_replicant_t *parent, btree_slice_t *slice);
void walk_branch(slice_walker_t *parent, block_id_t node);

btree_replicant_t::btree_replicant_t(store_t::replicant_t *cb, btree_key_value_store_t *s, repli_timestamp cutoff)
    : callback(cb), store(s), cutoff_recency(cutoff), stopping(false)
{
    active_slice_walkers = store->btree_static_config.n_slices;

    // Start walking all the slices so we can get information on keys that were inserted
    // before we started.
    for (int i = 0; i < store->btree_static_config.n_slices; i++) {
        walk_slice(this, store->slices[i]);
    }

    // Install ourselves as a trigger on each slice
    for (int i = 0; i < store->btree_static_config.n_slices; i++) {
        do_on_thread(store->slices[i]->home_thread, this, &btree_replicant_t::install, store->slices[i]);
    }
}

void btree_replicant_t::slice_walker_done() {
    active_slice_walkers--;
    assert(active_slice_walkers >= 0);

    // If the shutdown was blocked on the slice walkers, unblock it
    if (stopping && active_slice_walkers == 0 && active_uninstallations == 0) {
        done();
    }
}

bool btree_replicant_t::install(btree_slice_t *slice) {
    slice->replicants.push_back(this);
    return true;
}

void btree_replicant_t::stop() {
    assert_thread();
    stopping = true;

    // Uninstall our triggers
    active_uninstallations = store->btree_static_config.n_slices;
    for (int i = 0; i < store->btree_static_config.n_slices; i++) {
        do_on_thread(store->slices[i]->home_thread, this, &btree_replicant_t::uninstall, store->slices[i]);
    }
}

bool btree_replicant_t::uninstall(btree_slice_t *slice) {
    assert(stopping);

    std::vector<btree_replicant_t *>::iterator it;
    for (it = slice->replicants.begin(); it != slice->replicants.end(); it++) {
        if (*it == this) {
            slice->replicants.erase(it);
            do_on_thread(home_thread, this, &btree_replicant_t::have_uninstalled);
            return true;
        }
    }
    fail_due_to_user_error("We were never installed on this slice.");
}

bool btree_replicant_t::have_uninstalled() {
    assert(stopping);
    active_uninstallations--;
    assert(active_uninstallations >= 0);
    if (active_uninstallations == 0 && active_slice_walkers == 0) {
        done();
    }
    return true;
}

void btree_replicant_t::done() {
    assert(stopping);
    callback->stopped();
    delete this;
}

/* slice_walker_t walks a single btree and visits all the leaves. It reports the results
to a replicant. */

struct slice_walker_t :
    public home_thread_mixin_t,
    public block_available_callback_t
{
    btree_slice_t *slice;
    int active_branch_walkers;
    transaction_t *txn;
    btree_replicant_t *parent;
    slice_walker_t(btree_replicant_t *parent_, btree_slice_t *slice_)
        : slice(slice_), active_branch_walkers(0), parent(parent_)
    {
        do_on_thread(slice->home_thread, this, &slice_walker_t::start);
    }
    bool start() {
        txn = slice->cache.begin_transaction(rwi_read, NULL);
        assert(txn);   // Read-only transactions begin right away
        buf_t *buf = txn->acquire(SUPERBLOCK_ID, rwi_read, this);
        if (buf) on_block_available(buf);
        return true;
    }
    void on_block_available(buf_t *buf) {
        const btree_superblock_t *superblock = ptr_cast<btree_superblock_t>(buf->get_data_read());
        if (superblock->root_block != NULL_BLOCK_ID) {
            walk_branch(this, superblock->root_block);
        } else {
            done();
        }
        buf->release();
    }
    void done() {
        bool committed __attribute__((unused)) = txn->commit(NULL);
        assert(committed);
        do_on_thread(home_thread, this, &slice_walker_t::report);
    }
    bool report() {
        parent->slice_walker_done();
        delete this;
        return true;
    }
};

void walk_slice(btree_replicant_t *parent, btree_slice_t *slice) {
    new slice_walker_t(parent, slice);
}

/* branch_walker_t walks one branch of a btree. */

struct branch_walker_t :
    public block_available_callback_t,
    public large_buf_available_callback_t,
    public store_t::replicant_t::done_callback_t
{
    slice_walker_t *parent;
    buf_t *buf;
    
    int current_pair;   // Used for iterating over leaf nodes
    const store_key_t *current_key;
    const btree_value *current_value;
    repli_timestamp current_timestamp;
    large_buf_t *large_value;
    const_buffer_group_t buffers;
    
    branch_walker_t(slice_walker_t *parent, block_id_t block_id)
        : parent(parent), buf(NULL)
    {
        parent->active_branch_walkers++;
        buf_t *node = parent->txn->acquire(block_id, rwi_read, this);
        if (node) on_block_available(node);
    }
    
    ~branch_walker_t() {
        buf->release();
        parent->active_branch_walkers--;
        if (parent->active_branch_walkers == 0) {
            parent->done();
        }
    }
    
    void on_block_available(buf_t *b) {
        buf = b;
        if (node::is_internal(ptr_cast<node_t>(buf->get_data_read()))) {
            const internal_node_t *node = ptr_cast<internal_node_t>(buf->get_data_read());
            for (int i = 0; i < (int)node->npairs; i++) {
                const btree_internal_pair *pair = internal_node::get_pair(node, node->pair_offsets[i]);
                walk_branch(parent, pair->lnode);
            }
            delete this;
        } else {
            current_pair = -1;
            large_value = NULL;
            have_copied_value();
        }
    }
    
    void have_copied_value() {
        if (large_value) {
            large_value->release();
            delete large_value;
        }
        
        current_pair++;
        
        const leaf_node_t *node = ptr_cast<leaf_node_t>(buf->get_data_read());
        if (current_pair == node->npairs) {
            delete this;
        } else {
            current_timestamp = leaf::get_timestamp_value(parent->txn->cache->get_block_size(), node, current_pair);

            // TODO: right now we're walking through in order of key.
            // If we walked through in order of offset (by starting at
            // frontmost_offset and skipping through) we could
            // shortcircuit timestamps (because timestamp values are
            // monotonically decreasing with respect to that
            // ordering).

            if (repli_compare(current_timestamp, parent->parent->cutoff_recency) >= 0) {
                const btree_leaf_pair *pair = leaf::get_pair(node, node->pair_offsets[current_pair]);
                current_key = &pair->key;
                current_value = pair->value();

                if (current_value->is_large()) {
                    large_value = new large_buf_t(parent->txn);
                    large_value->acquire(current_value->lb_ref(), rwi_read, this);
                } else {
                    large_value = NULL;
                    buffers.buffers.clear();
                    buffers.add_buffer(current_value->value_size(), current_value->value());
                    deliver_value();
                }
            }
        }
    }
    
    void on_large_buf_available(large_buf_t *) {
        buffers.buffers.clear();
        for (int64_t i = 0; i < large_value->get_num_segments(); i++) {
            uint16_t size;
            const void *data = large_value->get_segment(i, &size);
            buffers.add_buffer(size, data);
        }
        deliver_value();
    }
    
    void deliver_value() {
        parent->parent->callback->value(current_key, &buffers, this,
                                        current_value->mcflags(), current_value->exptime(),
                                        current_value->has_cas() ? current_value->cas() : 0, current_timestamp);
    }
};

void walk_branch(slice_walker_t *parent, block_id_t node) {
    repli_timestamp subtree_recency = parent->txn->get_subtree_recency(node);
    if (repli_compare(subtree_recency, parent->parent->cutoff_recency) >= 0) {
        // The subtree is newer than or equal to the cutoff time.  It
        // has new stuff.  Walk it.
        new branch_walker_t(parent, node);
    }
}
