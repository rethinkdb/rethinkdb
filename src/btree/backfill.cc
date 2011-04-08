#include "btree/backfill.hpp"

#include "errors.hpp"
#include <algorithm>
#include <vector>
#include <boost/make_shared.hpp>

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/transactor.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/slice.hpp"

// Backfilling

// We want a backfill operation to follow a few simple rules.
//
// 1. Get as far away from the root as possible.
//
// 2. Avoid using more than K + O(1) blocks, for some user-selected
// constant K.
//
// 3. Prefetch efficiently.
//
// This code hopefully will be nice to genericize; you could
// reimplement rget if you genericized this.

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

// TODO: Actually use shutdown_mode.
class backfill_state_t {
public:
    backfill_state_t(const thread_saver_t& saver, btree_slice_t *_slice, repli_timestamp _since_when, backfill_callback_t *_callback, repli_timestamp _oper_start_timestamp)
        : oper_start_timestamp(_oper_start_timestamp), slice(_slice), since_when(_since_when),
          transactor_ptr(boost::make_shared<transactor_t>(saver, _slice->cache(), rwi_read, _since_when)),
          callback(_callback), shutdown_mode(false) { }

    repli_timestamp oper_start_timestamp;

    // The slice we're backfilling from.
    btree_slice_t *const slice;
    // The time from which we're backfilling.
    repli_timestamp const since_when;
    boost::shared_ptr<transactor_t> transactor_ptr;
    // The callback which receives key/value pairs.
    backfill_callback_t *const callback;
    cond_t finished_cond;

    // Should we stop backfilling immediately?
    bool shutdown_mode;

    // The number of pending + acquired blocks, by level.
    std::vector<int64_t> level_counts;

    int64_t& level_count(int level) {
        rassert(level >= 0);
        if (level >= int(level_counts.size())) {
            rassert(level == int(level_counts.size()), "Somehow we skipped a level! (level = %d, slice = %p)", level, slice);
            level_counts.resize(level + 1, 0);
        }
        return level_counts[level];
    }

    static int64_t level_max(UNUSED int level) {
        // TODO: Make this much bigger, this is for testing.  Temporary.
        return 2;
    }

    void consider_pulsing() {
        // We try to do as many pulses as we can (thus getting
        // behavior equivalent to calling consider_pulsing) but we
        // only actually do one pulse because this function gets
        // called immediately after every single block deallocation,
        // which only decrements counters by 1.

        // Right now we don't actually do more than one pulse at a
        // time, but we should try.

        for (int i = level_counts.size() - 1; i >= 0; --i) {
            if (level_counts[i] < level_max(i)) {
                int diff = level_max(i) - level_counts[i];

                while (diff > 0 && !acquisition_waiter_stacks[i].empty()) {
                    cond_t *cond = acquisition_waiter_stacks[i].back();
                    acquisition_waiter_stacks[i].pop_back();

                    cond->pulse();
                    diff -= 1;
                }
            }
        }

        if (total_level_count() == 0) {
            finished_cond.pulse();
        }
    }

    int64_t total_level_count() {
        int64_t sum = 0;
        for (int i = 0, n = level_counts.size(); i < n; ++i) {
            sum += level_counts[i];
        }
        return sum;
    }


    std::vector< std::vector<cond_t *> > acquisition_waiter_stacks;

    std::vector<cond_t *>& acquisition_waiter_stack(int level) {
        rassert(level >= 0);
        if (level >= int(acquisition_waiter_stacks.size())) {
            rassert(level == int(acquisition_waiter_stacks.size()), "Somehow we skipped a level! (level = %d, stacks.size() = %d, slice = %p)", level, int(acquisition_waiter_stacks.size()), slice);
            acquisition_waiter_stacks.resize(level + 1);
        }
        return acquisition_waiter_stacks[level];
    }

    void wait() {
        finished_cond.wait();
    }

private:
    DISABLE_COPYING(backfill_state_t);
};

void subtrees_backfill(backfill_state_t& state, buf_lock_t& parent, int level, block_id_t *block_ids, int num_block_ids);
void do_subtree_backfill(backfill_state_t& state, int level, block_id_t block_id, threadsafe_cond_t *acquisition_cond);
void process_leaf_node(backfill_state_t& state, buf_lock_t& buf_lock);
void process_internal_node(backfill_state_t& state, buf_lock_t& buf_lock, int level);


void get_recency_timestamps(backfill_state_t& state, block_id_t *block_ids, int num_block_ids, repli_timestamp *recencies_out);

// The main purpose of this type is to incr/decr state.level_counts.
class backfill_buf_lock_t {

public:
    backfill_buf_lock_t(const thread_saver_t& saver, backfill_state_t& state, int level, block_id_t block_id, threadsafe_cond_t *acquisition_cond)
        : state_(state), level_(level), inner_lock_() {
        acquire_node(saver, block_id, acquisition_cond);
    }

    ~backfill_buf_lock_t() {
        state_.level_count(level_) -= 1;
        state_.consider_pulsing();
    }

    void acquire_node(const thread_saver_t& saver, block_id_t block_id, threadsafe_cond_t *acquisition_cond) {
        // There are two ways to get permission to acquire.  One way:
        // breadth-first: if your grandparent and above are all zero.
        // If we are out of breadth-first credits or you don't
        // qualify, you can try for a depth-first credit, which are
        // given out rather liberally to people requesting nodes on
        // the bottom level seen so far, and automatically to people
        // requesting nodes beneath the existing bottom level.

        state_.level_count(level_) += 1;

        cond_t cond;
        state_.acquisition_waiter_stack(level_).push_back(&cond);

        state_.consider_pulsing();
        cond.wait();

        // Now actually acquire the node.
        buf_lock_t tmp(saver, state_.transactor_ptr->get(), block_id, rwi_read, acquisition_cond);
        inner_lock_.swap(tmp);
    }

    buf_t *operator->() { return inner_lock_.buf(); }
    buf_lock_t &inner_lock() { return inner_lock_; }

private:
    backfill_state_t& state_;
    int level_;
    buf_lock_t inner_lock_;
    DISABLE_COPYING(backfill_buf_lock_t);
};

perfmon_counter_t pm_backfill_coros("backfill_coros");


void btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback) {
    pm_backfill_coros++;
    thread_saver_t saver;
    backfill_state_t state(saver, slice, since_when, callback, current_time());
    buf_lock_t superblock_buf(saver, *state.transactor_ptr, SUPERBLOCK_ID, rwi_read);

    const btree_superblock_t *superblock = reinterpret_cast<const btree_superblock_t *>(superblock_buf->get_data_read());

    // TODO: The slave could sort the deletes and newer sets out, but
    // there are fairly few deletes.  We could (and should) release
    // the superblock lock much sooner than we do right now.

    // The deletes have to go first (since they get overridden by
    // newer sets)
    dump_keys_from_delete_queue(state.transactor_ptr, superblock->delete_queue_block, since_when, callback);

    block_id_t root_id = superblock->root_block;
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        // No root, so no keys in this entire shard.
        callback->done(state.oper_start_timestamp);
    } else {
        subtrees_backfill(state, superblock_buf, 0, &root_id, 1);
        state.wait();
        callback->done(state.oper_start_timestamp);
    }
    pm_backfill_coros--;
}

void subtrees_backfill(backfill_state_t& state, buf_lock_t& parent, int level, block_id_t *block_ids, int num_block_ids) {
    boost::scoped_array<repli_timestamp> recencies(new repli_timestamp[num_block_ids]);
    get_recency_timestamps(state, block_ids, num_block_ids, recencies.get());

    // Conds activated when we first try to acquire the children.
    // TODO: Replace acquisition_conds with a counter that counts down to zero.
    threadsafe_cond_t acquisition_countdown(num_block_ids);

    for (int i = 0; i < num_block_ids; ++i) {
        if (recencies[i].time >= state.since_when.time) {
            coro_t::spawn(boost::bind(do_subtree_backfill, boost::ref(state), level, block_ids[i], &acquisition_countdown));
        } else {
            acquisition_countdown.pulse();
        }
    }

    acquisition_countdown.wait();

    // The children are all pending acquisition; we can release the parent.
    parent.release();
}

void do_subtree_backfill(backfill_state_t& state, int level, block_id_t block_id, threadsafe_cond_t *acquisition_cond) {
    pm_backfill_coros++;
    thread_saver_t saver;
    backfill_buf_lock_t buf_lock(saver, state, level, block_id, acquisition_cond);

    const node_t *node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());

    if (node::is_leaf(node)) {
        process_leaf_node(state, buf_lock.inner_lock());
    } else {
        rassert(node::is_internal(node));

        process_internal_node(state, buf_lock.inner_lock(), level);
    }
    pm_backfill_coros--;
}

void process_internal_node(backfill_state_t& state, buf_lock_t& buf_lock, int level) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(buf_lock->get_data_read());

    boost::scoped_array<block_id_t> children;
    size_t num_children;
    internal_node::get_children_ids(node, children, &num_children);

    subtrees_backfill(state, buf_lock, level + 1, children.get(), num_children);
}

void process_leaf_node(backfill_state_t& state, buf_lock_t& buf_lock) {
    // This can be run in the scheduler thread.

    const leaf_node_t *node = reinterpret_cast<const leaf_node_t *>(buf_lock->get_data_read());

    // Remember, we only want to process recent keys.

    int npairs = node->npairs;

    for (int i = 0; i < npairs; ++i) {
        uint16_t offset = node->pair_offsets[i];
        repli_timestamp recency = leaf::get_timestamp_value(node, offset);

        if (recency.time >= state.since_when.time) {
            // The value is sufficiently recent.  But is it a small value or a large value?
            const btree_leaf_pair *pair = leaf::get_pair(node, offset);
            const btree_value *value = pair->value();
            unique_ptr_t<value_data_provider_t> data_provider(value_data_provider_t::create(value, state.transactor_ptr));
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
        }
    }
}

void get_recency_timestamps(backfill_state_t& state, block_id_t *block_ids, int num_block_ids, repli_timestamp *recencies_out) {
    // Uh... that was easy.
    (*state.transactor_ptr)->co_get_subtree_recencies(block_ids, num_block_ids, recencies_out);
}

