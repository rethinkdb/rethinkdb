#include "btree/backfill.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"


struct backfill_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_t {

    void process_a_leaf(transaction_t *txn, buf_t *leaf_node_buf, const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null) {
        assert_thread();
        const leaf_node_t *data = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        struct : public leaf::entry_reception_callback_t<void> {
            void lost_deletions() {
                cb->on_delete_range(left_exclusive_or_null, right_inclusive_or_null);
            }

            void deletion(const btree_key_t *k, repli_timestamp_t tstamp) {
                cb->on_deletion(k, tstamp);
            }

            void key_value(const btree_key_t *k, const void *value, repli_timestamp_t tstamp) {
                cb->on_pair(txn, tstamp, k, value);
            }

            agnostic_backfill_callback_t *cb;
            transaction_t *txn;
            const btree_key_t *left_exclusive_or_null;
            const btree_key_t *right_inclusive_or_null;
        } x;
        x.cb = callback_;
        x.txn = txn;
        x.left_exclusive_or_null = left_exclusive_or_null;
        x.right_inclusive_or_null = right_inclusive_or_null;

        leaf::dump_entries_since_time(sizer_, data, since_when_, maximum_possible_timestamp_, &x);
    }

    void postprocess_internal_node(UNUSED buf_t *internal_node_buf) {
        assert_thread();
        // do nothing
    }
    void postprocess_btree_superblock(UNUSED buf_t *superblock_buf) {
        assert_thread();
        // do nothing
    }

    access_t btree_superblock_mode() { return rwi_read; }
    access_t btree_node_mode() { return rwi_read; }

    struct annoying_t : public get_subtree_recencies_callback_t {
        interesting_children_callback_t *cb;
        boost::scoped_array<block_id_t> block_ids;
        ranged_block_ids_t *ids_source;
        boost::scoped_array<repli_timestamp_t> recencies;
        const key_range_t *key_range;
        repli_timestamp_t since_when;
        cond_t *done_cond;

        void got_subtree_recencies() {
            coro_t::spawn(boost::bind(&annoying_t::do_got_subtree_recencies, this));
        }

        void do_got_subtree_recencies() {
            rassert(coro_t::self());

            for (int i = 0, e = ids_source->num_block_ids(); i < e; ++i) {
                if (block_ids[i] != NULL_BLOCK_ID && recencies[i].time >= since_when.time) {
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
        fsm->block_ids.reset(new block_id_t[num_block_ids]);
        for (int i = 0; i < num_block_ids; ++i) {
            const btree_key_t *left, *right;
            block_id_t id;
            ids_source->get_block_id_and_bounding_interval(i, &id, &left, &right);
            if (overlaps(left, right, key_range_->left, key_range_->right)) {
                fsm->block_ids[i] = id;
            } else {
                fsm->block_ids[i] = NULL_BLOCK_ID;
            }
        }

        cond_t done_cond;
        fsm->cb = cb;
        fsm->ids_source = ids_source;
        fsm->key_range = key_range_;
        fsm->since_when = since_when_;
        fsm->recencies.reset(new repli_timestamp_t[num_block_ids]);
        fsm->done_cond = &done_cond;

        txn->get_subtree_recencies(fsm->block_ids.get(), num_block_ids, fsm->recencies.get(), fsm);
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

        if (!(x_right == NULL || sized_strcmp(y_left.contents, y_left.size, x_right->contents, x_right->size) <= 0)) {
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
            return sized_strcmp(x_left_copy.contents, x_left_copy.size, y_right.key.contents, y_right.key.size) < 0;
        }
    }


    agnostic_backfill_callback_t *callback_;
    repli_timestamp_t since_when_;
    repli_timestamp_t maximum_possible_timestamp_;
    value_sizer_t<void> *sizer_;
    const key_range_t *key_range_;

    backfill_traversal_helper_t(agnostic_backfill_callback_t *callback, repli_timestamp_t since_when,
                                repli_timestamp_t maximum_possible_timestamp,
                                value_sizer_t<void> *sizer, const key_range_t *key_range)
        : callback_(callback), since_when_(since_when), maximum_possible_timestamp_(maximum_possible_timestamp),
          sizer_(sizer), key_range_(key_range) { }
};


void do_agnostic_btree_backfill(value_sizer_t<void> *sizer, btree_slice_t *slice, const key_range_t *key_range, repli_timestamp_t since_when, repli_timestamp_t maximum_possible_timestamp, const boost::shared_ptr<cache_account_t>& backfill_account, agnostic_backfill_callback_t *callback, order_token_t token) {
    rassert(coro_t::self());

    backfill_traversal_helper_t helper(callback, since_when, maximum_possible_timestamp, sizer, key_range);

    slice->pre_begin_transaction_sink_.check_out(token);
    // TODO: Why are we using a write_mode source here?  There must be a reason...
    order_token_t begin_transaction_token = slice->pre_begin_transaction_write_mode_source_.check_in(token.tag() + "+begin_transaction_token").with_read_mode();

    transaction_t txn(slice->cache(), rwi_read_sync);

    txn.set_token(slice->post_begin_transaction_checkpoint_.check_through(begin_transaction_token));

#ifndef NDEBUG
    boost::scoped_ptr<assert_no_coro_waiting_t> no_coro_waiting(new assert_no_coro_waiting_t());
#endif

    txn.set_account(backfill_account);
    txn.snapshot();

#ifndef NDEBUG
    no_coro_waiting.reset();
#endif

    btree_parallel_traversal(&txn, slice, &helper);
}



class agnostic_memcached_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    explicit agnostic_memcached_backfill_callback_t(backfill_callback_t *cb) : cb_(cb) { }

    void on_delete_range(const btree_key_t *low, const btree_key_t *high) {
        cb_->on_delete_range(low, high);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency) {
        cb_->on_deletion(key, recency);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *val) {
        const memcached_value_t *value = static_cast<const memcached_value_t *>(val);
        boost::intrusive_ptr<data_buffer_t> data_provider = value_to_data_buffer(value, txn);
        backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = data_provider;
        atom.flags = value->mcflags();
        atom.exptime = value->exptime();
        atom.recency = recency;
        atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
        cb_->on_keyvalue(atom);
    }

    backfill_callback_t *cb_;
};


void btree_backfill(btree_slice_t *slice, repli_timestamp_t since_when, repli_timestamp_t maximum_possible_timestamp, const boost::shared_ptr<cache_account_t>& backfill_account, backfill_callback_t *callback, order_token_t token) {
    agnostic_memcached_backfill_callback_t agnostic_cb(callback);

    value_sizer_t<memcached_value_t> sizer(slice->cache()->get_block_size());
    store_key_t k;
    key_range_t range(key_range_t::none, k, key_range_t::none, k);
    do_agnostic_btree_backfill(&sizer, slice, &range, since_when, maximum_possible_timestamp, backfill_account, &agnostic_cb, token);
}
