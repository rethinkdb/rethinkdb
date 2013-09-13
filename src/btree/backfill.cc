// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/backfill.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/secondary_operations.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "protocol_api.hpp"

struct backfill_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_debug_only_t {
    void process_a_leaf(transaction_t *txn, buf_lock_t *leaf_node_buf, const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null, signal_t *interruptor, int * /*population_change_out*/) THROWS_ONLY(interrupted_exc_t) {
        assert_thread();
        const leaf_node_t *data = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        key_range_t clipped_range(
            left_exclusive_or_null ? key_range_t::open : key_range_t::none,
            left_exclusive_or_null ? store_key_t(left_exclusive_or_null) : store_key_t(),
            right_inclusive_or_null ? key_range_t::closed : key_range_t::none,
            right_inclusive_or_null ? store_key_t(right_inclusive_or_null) : store_key_t());
        clipped_range = clipped_range.intersection(key_range_);

        struct : public leaf::entry_reception_callback_t {
            void lost_deletions() {
                cb->on_delete_range(range, interruptor);
            }

            void deletion(const btree_key_t *k, repli_timestamp_t tstamp) {
                if (range.contains_key(k->contents, k->size)) {
                    cb->on_deletion(k, tstamp, interruptor);
                }
            }

            void key_value(const btree_key_t *k, const void *value, repli_timestamp_t tstamp) {
                if (range.contains_key(k->contents, k->size)) {
                    cb->on_pair(txn, tstamp, k, value, interruptor);
                }
            }

            agnostic_backfill_callback_t *cb;
            transaction_t *txn;
            key_range_t range;
            signal_t *interruptor;
        } x;
        x.cb = callback_;
        x.txn = txn;
        x.range = clipped_range;
        x.interruptor = interruptor;

        leaf::dump_entries_since_time(sizer_, data, since_when_, leaf_node_buf->get_recency(), &x);
    }

    void postprocess_internal_node(UNUSED buf_lock_t *internal_node_buf) {
        assert_thread();
        // do nothing
    }

    access_t btree_superblock_mode() { return rwi_read; }
    access_t btree_node_mode() { return rwi_read; }

    struct annoying_t : public get_subtree_recencies_callback_t {
        interesting_children_callback_t *cb;
        scoped_array_t<block_id_t> block_ids;
        scoped_array_t<repli_timestamp_t> recencies;
        repli_timestamp_t since_when;
        cond_t *done_cond;

        void got_subtree_recencies() {
            coro_t::spawn(boost::bind(&annoying_t::do_got_subtree_recencies, this));
        }

        void do_got_subtree_recencies() {
            rassert(coro_t::self());

            for (int i = 0, e = block_ids.size(); i < e; ++i) {
                if (block_ids[i] != NULL_BLOCK_ID && recencies[i] >= since_when) {
                    cb->receive_interesting_child(i);
                }
            }

            interesting_children_callback_t *local_cb = cb;
            cond_t *local_done_cond = done_cond;
            delete this;
            local_cb->no_more_interesting_children();
            local_done_cond->pulse();
        }
    };

    void filter_interesting_children(transaction_t *txn, ranged_block_ids_t *ids_source, interesting_children_callback_t *cb) {
        assert_thread();
        annoying_t *fsm = new annoying_t;
        int num_block_ids = ids_source->num_block_ids();
        fsm->block_ids.init(num_block_ids);
        for (int i = 0; i < num_block_ids; ++i) {
            const btree_key_t *left, *right;
            block_id_t id;
            ids_source->get_block_id_and_bounding_interval(i, &id, &left, &right);
            if (overlaps(left, right, key_range_.left, key_range_.right)) {
                fsm->block_ids[i] = id;
            } else {
                fsm->block_ids[i] = NULL_BLOCK_ID;
            }
        }

        cond_t done_cond;
        fsm->cb = cb;
        fsm->since_when = since_when_;
        fsm->recencies.init(num_block_ids);
        fsm->done_cond = &done_cond;

        txn->get_subtree_recencies(fsm->block_ids.data(), num_block_ids, fsm->recencies.data(), fsm);
        done_cond.wait();
    }

    // Checks if (x_left, x_right] intersects [y_left, y_right).  If
    // it returns false, the intersection may be non-empty.
    static bool overlaps(const btree_key_t *x_left, const btree_key_t *x_right,
                         const store_key_t& y_left, const key_range_t::right_bound_t& y_right) {
        // Does (x_left, x_right] intersects [y_left, y_right)?

        // For real numbers, if x_left < y_right and x_right >=
        // y_left, we have overlap.  However, for integers, consider
        // the case where x_left + 1 == y_right.  Then we don't have
        // overlap.  Our keys are like integers.

        if (!(x_right == NULL || sized_strcmp(y_left.contents(), y_left.size(), x_right->contents, x_right->size) <= 0)) {
            return false;
        }

        if (x_left == NULL || y_right.unbounded) {
            return true;
        } else {
            store_key_t x_left_copy(x_left->size, x_left->contents);
            if (!x_left_copy.increment()) {
                return false;
            }

            // Now it's [x_left_copy, x_right] intersecting [y_left, y_right).
            return sized_strcmp(x_left_copy.contents(), x_left_copy.size(), y_right.key.contents(), y_right.key.size()) < 0;
        }
    }


    agnostic_backfill_callback_t *callback_;
    repli_timestamp_t since_when_;
    value_sizer_t<void> *sizer_;
    const key_range_t& key_range_;

    backfill_traversal_helper_t(agnostic_backfill_callback_t *callback, repli_timestamp_t since_when,
                                value_sizer_t<void> *sizer, const key_range_t& key_range)
        : callback_(callback), since_when_(since_when), sizer_(sizer), key_range_(key_range) { }
};

void do_agnostic_btree_backfill(value_sizer_t<void> *sizer,
        btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when,
        agnostic_backfill_callback_t *callback, transaction_t *txn,
        superblock_t *superblock, buf_lock_t *sindex_block, parallel_traversal_progress_t *p,
        signal_t *interruptor)
THROWS_ONLY(interrupted_exc_t) {
    //Start things off easy with a coro assertion.
    rassert(coro_t::self());

    std::map<std::string, secondary_index_t> sindexes;
    get_secondary_indexes(txn, sindex_block, &sindexes);
    callback->on_sindexes(sindexes, interruptor);

    backfill_traversal_helper_t helper(callback, since_when, sizer, key_range);
    helper.progress = p;
    btree_parallel_traversal(txn, superblock, slice, &helper, interruptor);
}
