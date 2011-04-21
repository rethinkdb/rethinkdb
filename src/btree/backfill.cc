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

class backfill_state_t;
struct parent_releaser_t;

struct acquisition_start_callback_t {
    virtual void on_started_acquisition() = 0;
protected:
    ~acquisition_start_callback_t() { }
};

struct interesting_children_callback_t : public acquisition_start_callback_t {
    void receive_interesting_children(boost::scoped_array<block_id_t>& block_ids, int num_block_ids);

    void on_started_acquisition();
    void decr_acquisition_countdown();
    backfill_state_t *state;
    parent_releaser_t *releaser;
    int level;
    int acquisition_countdown;
};

struct btree_traversal_helper_t {
    // This is free to call mark_deleted.
    virtual void process_a_leaf(backfill_state_t *state, buf_t *leaf_node_buf) = 0;

    virtual void postprocess_internal_node(buf_t *internal_node_buf) = 0;
    virtual void postprocess_btree_superblock(buf_t *superblock_buf) = 0;

    virtual void filter_interesting_children(backfill_state_t *state, const block_id_t *block_ids, int num_block_ids, interesting_children_callback_t *cb);

    virtual access_t transaction_mode() = 0;

    virtual ~btree_traversal_helper_t() { }
};


btree_traversal_helper_t *HACK_make_backfill_traversal_helper(backfill_callback_t *callback, repli_timestamp since_when);

struct acquisition_waiter_callback_t {
    virtual void you_may_acquire() = 0;
protected:
    ~acquisition_waiter_callback_t() { }
};

// TODO: Actually use shutdown_mode.
class backfill_state_t {
public:
    backfill_state_t(const thread_saver_t& saver, btree_slice_t *_slice, repli_timestamp _since_when, btree_traversal_helper_t *_helper)
        : slice(_slice), since_when(_since_when),
          transactor_ptr(boost::make_shared<transactor_t>(saver, _slice->cache(), rwi_read, _since_when)),
          helper(_helper), shutdown_mode(false) { }

    // The slice we're backfilling from.
    btree_slice_t *const slice;
    // The time from which we're backfilling.
    repli_timestamp const since_when;
    boost::shared_ptr<transactor_t> transactor_ptr;
    // The callback which receives key/value pairs.
    boost::scoped_ptr<btree_traversal_helper_t> helper;
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
        // level = 1 is the root level.  These numbers are
        // ridiculously small because we have to spawn a coroutine
        // because the buffer cache is broken.
        if (level <= 3) {
            return 1000;
        } else {
            return 50;
        }
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

        for (int i = level_counts.size() - 1; i >= 0; --i) {
            if (level_counts[i] < level_max(i)) {
                int diff = level_max(i) - level_counts[i];

                while (diff > 0 && !acquisition_waiter_stacks[i].empty()) {
                    acquisition_waiter_callback_t *waiter_cb = acquisition_waiter_stacks[i].back();
                    acquisition_waiter_stacks[i].pop_back();

                    // For some reason the buffer cache is broken.  It
                    // expects transaction_t::acquire to run in a
                    // coroutine!  So we use coro_t::spawn instead of
                    // do_later.
                    coro_t::spawn(boost::bind(&acquisition_waiter_callback_t::you_may_acquire, waiter_cb));
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


    std::vector< std::vector<acquisition_waiter_callback_t *> > acquisition_waiter_stacks;

    std::vector<acquisition_waiter_callback_t *>& acquisition_waiter_stack(int level) {
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

struct backfill_traversal_helper_t : public btree_traversal_helper_t {
    void process_a_leaf(backfill_state_t *state, buf_t *leaf_node_buf) {
        const leaf_node_t *data = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        // Remember, we only want to process recent keys.

        int npairs = data->npairs;

        for (int i = 0; i < npairs; ++i) {
            uint16_t offset = data->pair_offsets[i];
            repli_timestamp recency = leaf::get_timestamp_value(data, offset);
            const btree_leaf_pair *pair = leaf::get_pair(data, offset);

            if (recency.time >= state->since_when.time) {
                const btree_value *value = pair->value();
                unique_ptr_t<value_data_provider_t> data_provider(value_data_provider_t::create(value, state->transactor_ptr));
                backfill_atom_t atom;
                atom.key.assign(pair->key.size, pair->key.contents);
                atom.value = data_provider;
                atom.flags = value->mcflags();
                atom.exptime = value->exptime();
                atom.recency = recency;
                atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
                callback_->on_keyvalue(atom);
            }
        }
    }

    void postprocess_internal_node(UNUSED buf_t *internal_node_buf) {
        // do nothing
    }
    void postprocess_btree_superblock(UNUSED buf_t *superblock_buf) {
        // do nothing
    }

    access_t transaction_mode() { return rwi_read; }

    struct annoying_t : public get_subtree_recencies_callback_t {
        interesting_children_callback_t *cb;
        boost::scoped_array<block_id_t> block_ids;
        int num_block_ids;
        boost::scoped_array<repli_timestamp> recencies;
        repli_timestamp since_when;

        void got_subtree_recencies() {
            boost::scoped_array<block_id_t> local_block_ids;
            local_block_ids.swap(block_ids);
            int j = 0;
            for (int i = 0; i < num_block_ids; ++i) {
                if (recencies[i].time >= since_when.time) {
                    local_block_ids[j] = local_block_ids[i];
                    ++j;
                }
            }
            int num_surviving_block_ids = j;

            delete this;
            coro_t::spawn(boost::bind(&interesting_children_callback_t::receive_interesting_children, cb, boost::ref(local_block_ids), num_surviving_block_ids));
        }
    };

    void filter_interesting_children(backfill_state_t *state, const block_id_t *block_ids, int num_block_ids, interesting_children_callback_t *cb) {
        annoying_t *fsm = new annoying_t;
        fsm->cb = cb;
        fsm->block_ids.reset(new block_id_t[num_block_ids]);
        std::copy(block_ids, block_ids + num_block_ids, fsm->block_ids.get());
        fsm->num_block_ids = num_block_ids;
        fsm->since_when = since_when_;
        fsm->recencies.reset(new repli_timestamp[num_block_ids]);

        state->transactor_ptr->get()->get_subtree_recencies(fsm->block_ids.get(), num_block_ids, fsm->recencies.get(), fsm);
    }

    backfill_callback_t *callback_;
    repli_timestamp since_when_;

    backfill_traversal_helper_t(backfill_callback_t *callback, repli_timestamp since_when)
        : callback_(callback), since_when_(since_when) { }
};


void subtrees_backfill(backfill_state_t *state, parent_releaser_t *releaser, int level, boost::scoped_array<block_id_t>& param_block_ids, int num_block_ids);
void do_a_subtree_backfill(backfill_state_t *state, int level, block_id_t block_id, acquisition_start_callback_t *acq_start_cb);
void process_a_leaf_node(backfill_state_t *state, buf_t *buf, int level);
void process_a_internal_node(backfill_state_t *state, buf_t *buf, int level);

struct node_ready_callback_t {
    virtual void on_node_ready(buf_t *buf) = 0;
protected:
    ~node_ready_callback_t() { }
};

struct acquire_a_node_fsm_t : public acquisition_waiter_callback_t, public block_available_callback_t {
    // Not much of an fsm.
    backfill_state_t *state;
    int level;
    block_id_t block_id;
    acquisition_start_callback_t *acq_start_cb;
    node_ready_callback_t *node_ready_cb;

    void you_may_acquire() {
        state->level_count(level) += 1;

        buf_t *buf = state->transactor_ptr->get()->acquire(block_id, rwi_read, this);
        // TODO: This is worthless crap.  We haven't started the
        // acquisition, we've FINISHED acquiring the buf because the
        // interface of acquire is a complete lie.  Thus we have worse
        // performance.
        acq_start_cb->on_started_acquisition();

        rassert(buf, "apparently the transaction_t::acquire interface now takes a callback parameter but never uses it");
        if (buf) {
            on_block_available(buf);
        }
    }

    void on_block_available(buf_t *block) {
        rassert(coro_t::self());
        node_ready_callback_t *local_cb = node_ready_cb;
        delete this;
        local_cb->on_node_ready(block);
    }
};


void acquire_a_node(backfill_state_t *state, int level, block_id_t block_id, acquisition_start_callback_t *acq_start_cb, node_ready_callback_t *node_ready_cb) {
    rassert(coro_t::self());
    acquire_a_node_fsm_t *fsm = new acquire_a_node_fsm_t;
    fsm->state = state;
    fsm->level = level;
    fsm->block_id = block_id;
    fsm->acq_start_cb = acq_start_cb;
    fsm->node_ready_cb = node_ready_cb;

    state->acquisition_waiter_stack(level).push_back(fsm);
    state->consider_pulsing();
}


struct parent_releaser_t {
    virtual void release() = 0;
protected:
    ~parent_releaser_t() { }
};

struct superblock_releaser_t : public parent_releaser_t {
    buf_t *buf_;
    backfill_state_t *state_;
    virtual void release() {
        state_->helper->postprocess_btree_superblock(buf_);
        buf_->release();
        delete this;
    }
    superblock_releaser_t(buf_t *buf, backfill_state_t *state) : buf_(buf), state_(state) { }
};

struct internal_node_releaser_t : public parent_releaser_t {
    buf_t *buf_;
    backfill_state_t *state_;
    virtual void release() {
        state_->helper->postprocess_internal_node(buf_);
        buf_->release();
        delete this;
    }
    internal_node_releaser_t(buf_t *buf, backfill_state_t *state) : buf_(buf), state_(state) { }
};

void btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback) {
    thread_saver_t saver;
    backfill_traversal_helper_t helper(callback, since_when);
    backfill_state_t state(saver, slice, since_when, &helper);
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
        callback->done();
    } else {
        boost::scoped_array<block_id_t> roots(new block_id_t[1]);
        roots[0] = root_id;
        state.level_count(0) += 1;
        state.acquisition_waiter_stacks.resize(1);
        subtrees_backfill(&state, new superblock_releaser_t(superblock_buf.give_up_ownership(), &state), 1, roots, 1);
        state.wait();
        callback->done();
    }
}

struct subtrees_backfill_fsm_t : public get_subtree_recencies_callback_t, public acquisition_start_callback_t {
    backfill_state_t *state;
    parent_releaser_t *releaser;
    int level;
    boost::scoped_array<block_id_t> block_ids;
    int num_block_ids;
    boost::scoped_array<repli_timestamp> recencies;
    int acquisition_countdown;

    void got_subtree_recencies() {
        coro_t::spawn_now(boost::bind(&subtrees_backfill_fsm_t::got_subtree_recencies_in_coro, this));
    }

    void got_subtree_recencies_in_coro() {
        rassert(coro_t::self());
        acquisition_countdown = num_block_ids + 1;

        for (int i = 0; i < num_block_ids; ++i) {
            if (recencies[i].time >= state->since_when.time) {
                do_a_subtree_backfill(state, level, block_ids[i], this);
            } else {
                decr_acquisition_countdown();
            }
        }

        decr_acquisition_countdown();
    }

    void on_started_acquisition() {
        rassert(coro_t::self());
        decr_acquisition_countdown();
    }

    void decr_acquisition_countdown() {
        rassert(coro_t::self());
        rassert(acquisition_countdown > 0);
        -- acquisition_countdown;
        if (acquisition_countdown == 0) {
            releaser->release();
            state->level_count(level - 1) -= 1;
            state->consider_pulsing();
            delete this;
        }
    }
};

void interesting_children_callback_t::receive_interesting_children(boost::scoped_array<block_id_t>& block_ids, int num_block_ids) {
    acquisition_countdown = num_block_ids + 1;

    for (int i = 0; i < num_block_ids; ++i) {
        do_a_subtree_backfill(state, level, block_ids[i], this);
    }

    decr_acquisition_countdown();
}


void interesting_children_callback_t::on_started_acquisition() {
    rassert(coro_t::self());
    decr_acquisition_countdown();
}

void interesting_children_callback_t::decr_acquisition_countdown() {
    rassert(coro_t::self());
    rassert(acquisition_countdown > 0);
    -- acquisition_countdown;
    if (acquisition_countdown == 0) {
        releaser->release();
        state->level_count(level - 1) -= 1;
        state->consider_pulsing();
        delete this;
    }
}


void subtrees_backfill(backfill_state_t *state, parent_releaser_t *releaser, int level, boost::scoped_array<block_id_t>& param_block_ids, int num_block_ids) {
    rassert(coro_t::self());
    interesting_children_callback_t *fsm = new interesting_children_callback_t;
    fsm->state = state;
    fsm->releaser = releaser;
    fsm->level = level;
    state->helper->filter_interesting_children(state, param_block_ids.get(), num_block_ids, fsm);
}

struct do_a_subtree_backfill_fsm_t : public node_ready_callback_t {
    backfill_state_t *state;
    int level;

    void on_node_ready(buf_t *buf) {
        rassert(coro_t::self());
        const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());

        backfill_state_t *local_state = state;
        int local_level = level;

        if (node::is_leaf(node)) {
            delete this;
            process_a_leaf_node(local_state, buf, local_level);
        } else {
            rassert(node::is_internal(node));

            delete this;
            process_a_internal_node(local_state, buf, local_level);
        }
    }
};

void do_a_subtree_backfill(backfill_state_t *state, int level, block_id_t block_id, acquisition_start_callback_t *acq_start_cb) {
    do_a_subtree_backfill_fsm_t *fsm = new do_a_subtree_backfill_fsm_t;
    fsm->state = state;
    fsm->level = level;

    acquire_a_node(state, level, block_id, acq_start_cb, fsm);
}

// This releases its buf_t parameter.
void process_a_internal_node(backfill_state_t *state, buf_t *buf, int level) {
    const internal_node_t *node = reinterpret_cast<const internal_node_t *>(buf->get_data_read());

    boost::scoped_array<block_id_t> children;
    size_t num_children;
    internal_node::get_children_ids(node, children, &num_children);

    subtrees_backfill(state, new internal_node_releaser_t(buf, state), level + 1, children, num_children);
}

// This releases its buf_t parameter.
void process_a_leaf_node(backfill_state_t *state, buf_t *buf, int level) {
    // This can be run in the scheduler thread.
    state->helper->process_a_leaf(state, buf);
    buf->release();
    state->level_count(level) -= 1;
    state->consider_pulsing();
}

