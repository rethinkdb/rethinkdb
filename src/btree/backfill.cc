#include "btree/backfill.hpp"

#include <algorithm>
#include <vector>
#include <boost/make_shared.hpp>

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/transactor.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/slice.hpp"

// TODO make this user-configurable.
#define BACKFILLING_MAX_BREADTH_FIRST_BLOCKS 50000
#define BACKFILLING_MAX_PENDING_BLOCKS 40000

// Backfilling

// We want a backfill operation to follow a few simple rules.
//
// 1. Get as far away from the root as possible.
//
// 2. Avoid using more than K + D blocks, for some user-selected
// constant K, where D is the depth of the tree.
//
// 3. Prefetch efficiently.
//
// This code should be nice to genericize; you could reimplement rget
// if you genericized this.  There are some performance things (like
// getting the recency of all an internal node's block ids at once)
// that need to be smart but are doable.


// The Lifecyle of a block_id_t
//
// Every time we deal with a block_id_t, it goes through these states...
//
// 1. Knowledge of the block_id_t.  This is where we know about the
// block_id_t, and haven't done anything about it yet.
//
// 2. Acquiring its subtree_recency value from the serializer.  The
// block_id_t is grouped with a bunch of others in an array, and we've
// sent a request to the serializer to respond with all these
// subtree_recency values (and the original array).
//
// 3. Acquired the subtree_recency value.  The block_id_t's
// subtree_recency is known, but we still have not attempted to
// acquire the block.  (If the recency is insufficiently recent, we
// stop here.)
//
// 4. Block acquisition pending.  We have sent a request to acquire
// the block.  It has not yet successfully completed.
//
// 5I. Block acquisition complete, it's an internal node, partly
// processed children.  We hold the lock on the block, and the
// children blocks are currently being processed and have not reached
// stage 4.
//
// 6I. Live children all reached stage 4.  We can now release ownership
// of the block.  We stop here.
//
// 5L. Block acquisition complete, it's a leaf node, we may have to
// handle large values.
//
// 6L. Large values all pending or better, so we can release ownership
// of the block.  We stop here.

class backfill_state_t {
public:
    backfill_state_t(btree_slice_t *_slice, repli_timestamp _since_when, backfill_callback_t *_callback)
        : slice(_slice), since_when(_since_when), transactor_ptr(boost::make_shared<transactor_t>(&_slice->cache(), rwi_read, _since_when)),
          callback(_callback), shutdown_mode(false), held_blocks(), num_pending_blocks(0) { }

    // The slice we're backfilling from.
    btree_slice_t *const slice;
    // The time from which we're backfilling.
    repli_timestamp const since_when;
    boost::shared_ptr<transactor_t> transactor_ptr;
    // The callback which receives key/value pairs.
    backfill_callback_t *const callback;

    // Should we stop backfilling immediately?
    bool shutdown_mode;

    // Blocks we currently hold, organized by level.
    std::vector< std::vector<buf_t *> > held_blocks;
    // The number of blocks we are currently loading.
    int num_pending_blocks;

private:
    DISABLE_COPYING(backfill_state_t);
};

int num_live(const backfill_state_t& state) {
    // 8 petabytes of RAM should be enough for backfilling
    int ret = 0;
    for (int i = 0, n = state.held_blocks.size(); i < n; ++i) {
        ret += state.held_blocks[i].size();
    }
    return ret + state.num_pending_blocks;
}

// Naive functions' declarations
void subtrees_backfill(backfill_state_t& state, buf_lock_t& parent, int level, block_id_t *block_ids, int num_block_ids);
void do_subtree_backfill(backfill_state_t& state, int level, block_id_t block_id, cond_t *acquisition_cond);
void process_leaf_node(backfill_state_t& state, buf_lock_t& lock);
void process_internal_node(backfill_state_t& state, buf_lock_t& lock, int level);

// Less naive functions' declarations
void get_recency_timestamps(backfill_state_t& state, block_id_t *block_ids, int num_block_ids, repli_timestamp *recencies_out);
void acquire_node(buf_lock_t& lock_to_initialize, backfill_state_t& state, int level, block_id_t block_id, cond_t *acquisition_cond);

void spawn_btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback) {
    backfill_state_t state(slice, since_when, callback);

    buf_lock_t buf_lock(*state.transactor_ptr, SUPERBLOCK_ID, rwi_read);
    block_id_t root_id = reinterpret_cast<const btree_superblock_t *>(buf_lock.buf()->get_data_read())->root_block;
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        // No root, so no keys in this entire shard.
        return;
    }

    subtrees_backfill(state, buf_lock, 0, &root_id, 1);
}

void subtrees_backfill(backfill_state_t& state, buf_lock_t& parent, int level, block_id_t *block_ids, int num_block_ids) {
    boost::scoped_array<repli_timestamp> recencies(new repli_timestamp[num_block_ids]);
    get_recency_timestamps(state, block_ids, num_block_ids, recencies.get());

    // Conds activated when we first try to acquire the children.
    // TODO: Replace acquisition_conds with a counter that counts down to zero.
    boost::scoped_array<cond_t> acquisition_conds(new cond_t[num_block_ids]);
    for (int i = 0; i < num_block_ids; ++i) {
        if (recencies[i].time >= state.since_when.time) {
            coro_t::spawn(boost::bind(do_subtree_backfill, boost::ref(state), level, block_ids[i], &acquisition_conds[i]));
        } else {
            acquisition_conds[i].pulse();
        }
    }

    for (int i = 0; i < num_block_ids; ++i) {
        acquisition_conds[i].wait();
    }

    // The children are all pending acquisition; we can release the parent.
    parent.release();
}

void do_subtree_backfill(backfill_state_t& state, int level, block_id_t block_id, cond_t *acquisition_cond) {
    buf_lock_t buf_lock;
    acquire_node(buf_lock, state, level, block_id, acquisition_cond);

    const node_t *node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());

    if (node::is_leaf(node)) {
        process_leaf_node(state, buf_lock);
    } else {
        rassert(node::is_internal(node));

        process_internal_node(state, buf_lock, level);
    }
}

void process_internal_node(backfill_state_t& state, buf_lock_t& buf_lock, int level) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(buf_lock->get_data_read());

    boost::scoped_array<block_id_t> children;
    size_t num_children;
    internal_node::get_children_ids(node, children, &num_children);

    subtrees_backfill(state, buf_lock, level + 1, children.get(), num_children);
}

void process_leaf_node(backfill_state_t& state, buf_lock_t& buf_lock) {
    const leaf_node_t *node = reinterpret_cast<const leaf_node_t *>(buf_lock->get_data_read());

    // Remember, we only want to process recent keys.

    int npairs = node->npairs;

    // TODO: Replace large_buf_acquisition_conds with an object that counts down to zero.
    boost::scoped_array<cond_t> large_buf_acquisition_conds(new cond_t[npairs]);

    for (int i = 0; i < npairs; ++i) {
        uint16_t offset = node->pair_offsets[i];
        repli_timestamp recency = leaf::get_timestamp_value(node, offset);

        if (recency.time >= state.since_when.time) {
            // The value is sufficiently recent.  But is it a small value or a large value?
            const btree_leaf_pair *pair = leaf::get_pair(node, offset);
            const btree_value *value = pair->value();
            value_data_provider_t *data_provider = value_data_provider_t::create(value, state.transactor_ptr, &large_buf_acquisition_conds[i]);
            backfill_atom_t atom;
            // We'd like to use keycpy but nooo we have store_key_t and btree_key as different types.
            atom.key.size = pair->key.size;
            memcpy(atom.key.contents, pair->key.contents, pair->key.size);
            atom.value = data_provider;
            atom.flags = value->mcflags();
            atom.exptime = value->exptime();
            atom.recency = recency;
            atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
            state.callback->on_keyvalue(atom);
        } else {
            large_buf_acquisition_conds[i].pulse();
        }
    }

    for (int i = 0; i < npairs; ++i) {
        large_buf_acquisition_conds[i].wait();
    }
}

void get_recency_timestamps(backfill_state_t& state, block_id_t *block_ids, int num_block_ids, repli_timestamp *recencies_out) {
    // Uh... that was easy.
    (*state.transactor_ptr)->get_subtree_recencies(block_ids, num_block_ids, recencies_out);
}

// This looks at state and can block for a long time.
void acquire_node(buf_lock_t& lock_to_initialize, backfill_state_t& state, int level, block_id_t block_id, cond_t *acquisition_cond) {
    

}
