// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/parallel_traversal.hpp"

#include "arch/runtime/runtime.hpp"
#include "btree/internal_node.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"

// By the way, this code all sucks, the design of this code sucks, and the entire
// premise upon which this code was designed was retarded.
//
//                                              - he/she that wrote this code

// Traversal

// We want a traversal operation to follow a few simple rules.
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

// The Lifecycle of a block_id_t
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
//
// (6L might not be implemented, or 5L and 6L might depend on whether
// the btree_traversal_helper_t implementation is interested in
// values.)

class parent_releaser_t;

struct acquisition_waiter_callback_t {
    virtual void you_may_acquire() = 0;
    virtual void cancel() = 0;
protected:
    virtual ~acquisition_waiter_callback_t() { }
};

class traversal_state_t : public coro_pool_callback_t<acquisition_waiter_callback_t *> {
public:
    traversal_state_t(block_size_t _max_block_size, btree_traversal_helper_t *_helper,
                      signal_t *_interruptor)
        : max_block_size(_max_block_size),
          stat_block(NULL_BLOCK_ID),
          helper(_helper),
          interruptor(_interruptor),
          interrupted(false),
          coro_pool(4, &pending_acquires, this)
    {
        interruptor_watcher.parent = this;
        interruptor_watcher.reset(_interruptor);
    }

    ~traversal_state_t() {
        if (!interrupted) {
            for (size_t i = 0; i < acquisition_waiter_stacks.size(); ++i) {
                for (size_t j = 0; j < acquisition_waiter_stacks[i].size(); ++j) {
                    acquisition_waiter_stacks[i][j]->cancel();
                }
            }
        }
    }

    const block_size_t max_block_size;

    /* The block id where we can find the stat block, we need this at the end
     * to update population counts. */
    block_id_t stat_block;

    // The helper.
    btree_traversal_helper_t *helper;

    signal_t *interruptor;
    bool interrupted;
    cond_t finished_cond;

    // The number of pending + acquired blocks, by level.
    std::vector<int64_t> level_counts;

    class interruptor_watcher_t : public signal_t::subscription_t {
    public:
        void run() {
            parent->interrupt();
        }
        traversal_state_t *parent;
    } interruptor_watcher;

    int64_t& level_count(int level) {
        rassert(level >= 0);
        if (size_t(level) >= level_counts.size()) {
            rassert(size_t(level) == level_counts.size(), "Somehow we skipped a level! (level = %d)", level);
            level_counts.resize(level + 1, 0);
        }
        return level_counts[level];
    }

    static int64_t level_max(UNUSED int level) {
        // level = 1 is the root level.  These numbers are
        // ridiculously small because we have to spawn a coroutine
        // because the buffer cache is broken.
        // Also we tend to interfere with other i/o for a weird reason.
        // (potential explanation: on the higher levels of the btree, we
        // trigger the load of a significant fraction of all blocks. Sooner or
        // later, queries end up waiting for the same blocks. But at that point
        // they effectively get queued into our i/o queue (assuming we have an
        // i/o account of ourselves). If this number is high and disks are slow,
        // the latency of that i/o queue can be in the area of seconds though,
        // and the query blocks for that time).
        return 16;
    }

    void consider_pulsing() {
        rassert(coro_t::self());

        // We try to do as many pulses as we can (thus getting
        // behavior equivalent to calling consider_pulsing) but we
        // only actually do one pulse because this function gets
        // called immediately after every single block deallocation,
        // which only decrements counters by 1.

        // Right now we don't actually do more than one pulse at a
        // time, but we should try.

        rassert(level_counts.size() <= acquisition_waiter_stacks.size());
        level_counts.resize(acquisition_waiter_stacks.size(), 0);

        // If we're trying to stop, don't spawn any new jobs.
        if (!interrupted) {
            for (int i = level_counts.size() - 1; i >= 0; --i) {
                if (level_counts[i] < level_max(i)) {
                    int diff = level_max(i) - level_counts[i];

                    while (diff > 0 && !acquisition_waiter_stacks[i].empty()) {
                        acquisition_waiter_callback_t *waiter_cb = acquisition_waiter_stacks[i].back();
                        acquisition_waiter_stacks[i].pop_back();

                        // Spawn a coroutine so that it's safe to acquire
                        // blocks.
                        level_count(i) += 1;
                        pending_acquires.push(waiter_cb);
                        diff -= 1;
                    }
                }
            }
        }

        if (total_level_count() == 0) {
            finished_cond.pulse();
        }
    }

    void coro_pool_callback(acquisition_waiter_callback_t *waiter_cb, UNUSED signal_t *pool_interruptor) {
        waiter_cb->you_may_acquire();
    }

    void interrupt() {
        rassert(interrupted == false);
        interrupted = true;
        for (size_t i = 0; i < acquisition_waiter_stacks.size(); ++i) {
            for (size_t j = 0; j < acquisition_waiter_stacks[i].size(); ++j) {
                acquisition_waiter_stacks[i][j]->cancel();
            }
        }
    }

    int64_t total_level_count() {
        int64_t sum = 0;
        for (size_t i = 0; i < level_counts.size(); ++i) {
            sum += level_counts[i];
        }
        return sum;
    }

    std::vector< std::vector<acquisition_waiter_callback_t *> > acquisition_waiter_stacks;

    std::vector<acquisition_waiter_callback_t *>& acquisition_waiter_stack(int level) {
        rassert(level >= 0);
        if (level >= static_cast<int>(acquisition_waiter_stacks.size())) {
            rassert(level == static_cast<int>(acquisition_waiter_stacks.size()),
                    "Somehow we skipped a level! (level = %d, stacks.size() = %d)",
                    level, static_cast<int>(acquisition_waiter_stacks.size()));
            acquisition_waiter_stacks.resize(level + 1);
        }
        return acquisition_waiter_stacks[level];
    }

    void wait() {
        finished_cond.wait();
    }

private:
    unlimited_fifo_queue_t<acquisition_waiter_callback_t *> pending_acquires;
    coro_pool_t<acquisition_waiter_callback_t *> coro_pool;
    DISABLE_COPYING(traversal_state_t);
};

void subtrees_traverse(traversal_state_t *state,
                       parent_releaser_t *releaser, int level,
                       const counted_t<ranged_block_ids_t> &ids_source);
void do_a_subtree_traversal(traversal_state_t *state, int level,
                            buf_parent_t parent, block_id_t block_id,
                            btree_key_t *left_exclusive_or_null,
                            btree_key_t *right_inclusive_or_null,
                            lock_in_line_callback_t *acq_start_cb);

void process_a_leaf_node(traversal_state_t *state,
                         buf_lock_t buf, int level,
                         const btree_key_t *left_exclusive_or_null,
                         const btree_key_t *right_inclusive_or_null);
void process_a_internal_node(traversal_state_t *state,
                             buf_lock_t buf, int level,
                             const btree_key_t *left_exclusive_or_null,
                             const btree_key_t *right_inclusive_or_null);

struct node_ready_callback_t {
    virtual void on_node_ready(buf_lock_t buf) = 0;
    virtual void on_cancel() = 0;
protected:
    virtual ~node_ready_callback_t() { }
};

struct acquire_a_node_fsm_t : public acquisition_waiter_callback_t {
    // Not much of an fsm.
    traversal_state_t *state;
    buf_parent_t parent;
    int level;
    block_id_t block_id;
    lock_in_line_callback_t *acq_start_cb;
    node_ready_callback_t *node_ready_cb;

    explicit acquire_a_node_fsm_t(buf_parent_t _parent) : parent(_parent) { }

    void you_may_acquire() {
        rassert(coro_t::self());

        buf_lock_t block(parent, block_id, state->helper->btree_node_mode());
        acq_start_cb->on_in_line();

        node_ready_callback_t *local_cb = node_ready_cb;
        delete this;
        local_cb->on_node_ready(std::move(block));
    }

    void cancel() {
        /* This is safe because `on_in_line()` is used only to know when it's OK
        to release the parent, and now that we're not gonna ever acquire the
        child it's safe to release the parent. */
        acq_start_cb->on_in_line();

        node_ready_callback_t *local_cb = node_ready_cb;
        delete this;
        local_cb->on_cancel();
    }
};


void acquire_a_node(traversal_state_t *state, int level,
                    buf_parent_t parent, block_id_t block_id,
                    lock_in_line_callback_t *acq_start_cb,
                    node_ready_callback_t *node_ready_cb) {
    rassert(coro_t::self());
    acquire_a_node_fsm_t *fsm = new acquire_a_node_fsm_t(parent);
    fsm->state = state;
    fsm->level = level;
    fsm->block_id = block_id;
    fsm->acq_start_cb = acq_start_cb;
    fsm->node_ready_cb = node_ready_cb;

    state->acquisition_waiter_stack(level).push_back(fsm);

    if (state->interrupted) {
        fsm->cancel();
    }

    state->consider_pulsing();
}

class parent_releaser_t {
public:
    virtual buf_parent_t expose_parent() = 0;
    virtual void release() = 0;
protected:
    virtual ~parent_releaser_t() { }
};

struct internal_node_releaser_t : public parent_releaser_t {
    buf_lock_t buf_;
    traversal_state_t *state_;
    buf_parent_t expose_parent() {
        return buf_parent_t(&buf_);
    }
    virtual void release() {
        state_->helper->postprocess_internal_node(&buf_);
        delete this;
    }
    internal_node_releaser_t(buf_lock_t &&buf,
                             traversal_state_t *state)
        : buf_(std::move(buf)),
          state_(state) { }

    virtual ~internal_node_releaser_t() { }
};

void btree_parallel_traversal(superblock_t *superblock,
                              btree_traversal_helper_t *helper,
                              signal_t *interruptor,
                              release_superblock_t release_superblock)
    THROWS_ONLY(interrupted_exc_t) {
    traversal_state_t state(superblock->cache()->max_block_size(),
                            helper, interruptor);

    /* Record the stat block for updating populations later */
    state.stat_block = superblock->get_stat_block_id();

    if (state.stat_block != NULL_BLOCK_ID) {
        /* Give the helper a look at the stat block */
        buf_lock_t stat_block(buf_parent_t(superblock->expose_buf().txn()),
                              state.stat_block, access_t::read);
        helper->read_stat_block(&stat_block);
    } else {
        helper->read_stat_block(NULL);
    }

    if (helper->progress) {
        helper->progress->inform(0, parallel_traversal_progress_t::LEARN, parallel_traversal_progress_t::INTERNAL);
        helper->progress->inform(0, parallel_traversal_progress_t::ACQUIRE, parallel_traversal_progress_t::INTERNAL);
    }

    block_id_t root_id = superblock->get_root_block_id();
    rassert(root_id != SUPERBLOCK_ID);

    struct : public parent_releaser_t {
        superblock_t *superblock;
        bool release_superblock;
        buf_parent_t expose_parent() {
            return superblock->expose_buf();
        }
        void release() {
            if (release_superblock) {
                superblock->release();
            }
        }
    } superblock_releaser;
    superblock_releaser.superblock = superblock;
    superblock_releaser.release_superblock =
        release_superblock == release_superblock_t::RELEASE;

    if (root_id == NULL_BLOCK_ID) {
        superblock_releaser.release();
        // No root, so no keys in this entire shard.
    } else {
        state.level_count(0) += 1;
        state.acquisition_waiter_stacks.resize(1);
        counted_t<ranged_block_ids_t> ids_source(new ranged_block_ids_t(root_id, NULL, NULL, 0));
        subtrees_traverse(&state,
                          &superblock_releaser, 1, ids_source);
        state.wait();
        /* Wait for `state.wait()` to succeed even if `interruptor` is pulsed,
        so that we don't abandon running coroutines or FSMs */
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
}


void subtrees_traverse(traversal_state_t *state,
                       parent_releaser_t *releaser,
                       int level,
                       const counted_t<ranged_block_ids_t>& ids_source) {
    rassert(coro_t::self());
    interesting_children_callback_t *fsm
        = new interesting_children_callback_t(state, releaser, level,
                                              ids_source);
    state->helper->filter_interesting_children(releaser->expose_parent(),
                                               ids_source.get(), fsm);
}

struct do_a_subtree_traversal_fsm_t : public node_ready_callback_t {
    traversal_state_t *state;
    int level;
    store_key_t left_exclusive;
    bool left_unbounded;
    store_key_t right_inclusive;
    bool right_unbounded;

    void on_node_ready(buf_lock_t buf) {
        rassert(coro_t::self());
        bool is_leaf;
        {
            buf_read_t read(&buf);
            const node_t *node = static_cast<const node_t *>(read.get_data_read());
            is_leaf = node::is_leaf(node);
        }

        const btree_key_t *left_exclusive_or_null = left_unbounded ? NULL : left_exclusive.btree_key();
        const btree_key_t *right_inclusive_or_null = right_unbounded ? NULL : right_inclusive.btree_key();

        if (is_leaf) {
            if (state->helper->progress) {
                state->helper->progress->inform(level, parallel_traversal_progress_t::ACQUIRE, parallel_traversal_progress_t::LEAF);
            }
            process_a_leaf_node(state, std::move(buf), level,
                                left_exclusive_or_null, right_inclusive_or_null);
        } else {
            // The node is internal
            if (state->helper->progress) {
                state->helper->progress->inform(level, parallel_traversal_progress_t::ACQUIRE, parallel_traversal_progress_t::INTERNAL);
            }
            process_a_internal_node(state, std::move(buf), level,
                                    left_exclusive_or_null, right_inclusive_or_null);
        }

        delete this;
    }

    void on_cancel() {
        delete this;
    }
};

void do_a_subtree_traversal(traversal_state_t *state, int level,
                            buf_parent_t parent, block_id_t block_id,
                            const btree_key_t *left_exclusive_or_null,
                            const btree_key_t *right_inclusive_or_null,
                            lock_in_line_callback_t *acq_start_cb) {
    do_a_subtree_traversal_fsm_t *fsm = new do_a_subtree_traversal_fsm_t;
    fsm->state = state;
    fsm->level = level;

    if (left_exclusive_or_null) {
        fsm->left_exclusive.assign(left_exclusive_or_null);
        fsm->left_unbounded = false;
    } else {
        fsm->left_unbounded = true;
    }
    if (right_inclusive_or_null) {
        fsm->right_inclusive.assign(right_inclusive_or_null);
        fsm->right_unbounded = false;
    } else {
        fsm->right_unbounded = true;
    }

    acquire_a_node(state, level, parent, block_id, acq_start_cb, fsm);
}

// This releases its buf_lock_t parameter.
void process_a_internal_node(traversal_state_t *state,
                             buf_lock_t buf,
                             int level,
                             const btree_key_t *left_exclusive_or_null,
                             const btree_key_t *right_inclusive_or_null) {
    counted_t<ranged_block_ids_t> ids_source;
    {
        buf_read_t read(&buf);
        const internal_node_t *node
            = static_cast<const internal_node_t *>(read.get_data_read());

        ids_source = make_counted<ranged_block_ids_t>(
                    state->max_block_size, node,
                    left_exclusive_or_null, right_inclusive_or_null, level);
    }

    subtrees_traverse(state, new internal_node_releaser_t(std::move(buf), state),
                      level + 1, ids_source);
}

void process_a_leaf_node(traversal_state_t *state, buf_lock_t buf,
        int level, const btree_key_t *left_exclusive_or_null,
        const btree_key_t *right_inclusive_or_null) {
    rassert(coro_t::self());
    int population_change = 0;

    try {
        state->helper->process_a_leaf(&buf, left_exclusive_or_null,
                right_inclusive_or_null, state->interruptor, &population_change);
    } catch (const interrupted_exc_t &) {
        rassert(state->interruptor->is_pulsed());
        /* ignore it; the backfill will come to a stop on its own now that
        `interruptor` has been pulsed */
    }

    txn_t *txn = buf.txn();
    buf.reset_buf_lock();

    if (state->helper->btree_node_mode() != access_t::write) {
        rassert(population_change == 0, "A read only operation claims it change the population of a leaf.\n");
    } else if (population_change != 0 && state->stat_block != NULL_BLOCK_ID) {
        // The stat block has no parent, our changes to it are commutative and
        // readers expect out-of-date values.
        buf_lock_t stat_block(buf_parent_t(txn), state->stat_block, access_t::write);
        buf.reset_buf_lock();
        buf_write_t stat_block_write(&stat_block);
        auto stat_block_buf = static_cast<btree_statblock_t *>(
                stat_block_write.get_data_write(BTREE_STATBLOCK_SIZE));
        stat_block_buf->population += population_change;
    } else {
        // Don't acquire the block to not change the value.
    }

    if (state->helper->progress) {
        state->helper->progress->inform(level, parallel_traversal_progress_t::RELEASE, parallel_traversal_progress_t::LEAF);
    }
    state->level_count(level) -= 1;
    state->consider_pulsing();
}

void interesting_children_callback_t::receive_interesting_child(int child_index) {
    rassert(child_index >= 0 && child_index < ids_source->num_block_ids());

    if (state->helper->progress) {
        state->helper->progress->inform(level, parallel_traversal_progress_t::LEARN, parallel_traversal_progress_t::UNKNOWN);
    }
    block_id_t block_id;
    const btree_key_t *left_excl_or_null;
    const btree_key_t *right_incl_or_null;
    ids_source->get_block_id_and_bounding_interval(child_index, &block_id, &left_excl_or_null, &right_incl_or_null);

    ++acquisition_countdown;
    do_a_subtree_traversal(state, level, releaser->expose_parent(),
                           block_id, left_excl_or_null, right_incl_or_null, this);
}

void interesting_children_callback_t::no_more_interesting_children() {
    decr_acquisition_countdown();
}

void interesting_children_callback_t::on_in_line() {
    decr_acquisition_countdown();
}

void interesting_children_callback_t::decr_acquisition_countdown() {
    rassert(coro_t::self());
    rassert(acquisition_countdown > 0);
    --acquisition_countdown;
    if (acquisition_countdown == 0) {
        releaser->release();
        state->level_count(level - 1) -= 1;
        if (state->helper->progress) {
            state->helper->progress->inform(level - 1, parallel_traversal_progress_t::RELEASE, parallel_traversal_progress_t::INTERNAL);
        }
        state->consider_pulsing();
        delete this;
    }
}


int ranged_block_ids_t::num_block_ids() const {
    if (node_.has()) {
        return node_->npairs;
    } else {
        return 1;
    }
}

void ranged_block_ids_t::get_block_id_and_bounding_interval(int index,
                                                            block_id_t *block_id_out,
                                                            const btree_key_t **left_excl_bound_out,
                                                            const btree_key_t **right_incl_bound_out) const {
    if (node_.has()) {
        rassert(index >= 0);
        rassert(index < node_->npairs);

        const btree_internal_pair *pair = internal_node::get_pair_by_index(node_.get(), index);
        *block_id_out = pair->lnode;
        *right_incl_bound_out = (index == node_->npairs - 1 ? right_inclusive_or_null_ : &pair->key);

        if (index == 0) {
            *left_excl_bound_out = left_exclusive_or_null_;
        } else {
            const btree_internal_pair *left_neighbor = internal_node::get_pair_by_index(node_.get(), index - 1);
            *left_excl_bound_out = &left_neighbor->key;
        }
    } else {
        *block_id_out = forced_block_id_;
        *left_excl_bound_out = left_exclusive_or_null_;
        *right_incl_bound_out = right_inclusive_or_null_;
    }
}

int ranged_block_ids_t::get_level() {
    return level;
}

void parallel_traversal_progress_t::inform(int level, action_t action, node_type_t type) {
    assert_thread();
    rassert(learned.size() == acquired.size() && acquired.size() == released.size());
    rassert(level >= 0);
    if (size_t(level) >= learned.size()) {
        learned.resize(level + 1, 0);
        acquired.resize(level + 1, 0);
        released.resize(level + 1, 0);
    }

    if (type == LEAF) {
        if (height == -1) {
            height = level;
        }
        rassert(height == level);
    }

    switch (action) {
    case LEARN:
        learned[level]++;
        break;
    case ACQUIRE:
        acquired[level]++;
        break;
    case RELEASE:
        released[level]++;
        break;
    default:
        unreachable();
        break;
    }
    rassert(learned.size() == acquired.size() && acquired.size() == released.size());
}

progress_completion_fraction_t parallel_traversal_progress_t::guess_completion() const {
    assert_thread();
    rassert(learned.size() == acquired.size() && acquired.size() == released.size());

    if (height == -1) {
        return progress_completion_fraction_t();
    }

    /* First we compute the ratio at each stage of the acquired nodes to the
     * learned nodes. This gives us a rough estimate of the branch factor at
     * each level. */

    std::vector<double> released_to_acquired_ratios;
    for (unsigned i = 0; i + 1 < learned.size(); ++i) {
        released_to_acquired_ratios.push_back(static_cast<double>(acquired[i + 1]) / std::max<double>(1.0, released[i]));
    }

    std::vector<int> population_by_level_guesses;
    population_by_level_guesses.push_back(learned[0]);

    rassert(!learned.empty());
    for (unsigned i = 0; i + 1 < learned.size(); i++) {
        population_by_level_guesses.push_back(static_cast<int>(released_to_acquired_ratios[i] * population_by_level_guesses[i]));
    }

    int estimate_of_total_nodes = 0;
    int total_released_nodes = 0;
    for (unsigned i = 0; i < population_by_level_guesses.size(); i++) {
        estimate_of_total_nodes += population_by_level_guesses[i];
        total_released_nodes += released[i];
    }

    return progress_completion_fraction_t(total_released_nodes, estimate_of_total_nodes);
}

