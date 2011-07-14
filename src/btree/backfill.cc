#include "btree/backfill.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/coroutines.hpp"
#include "buffer_cache/co_functions.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"


class btree_slice_t;

struct backfill_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_t {

    // The deletes have to go first (since they get overridden by
    // newer sets)
    void preprocess_btree_superblock(transaction_t *txn, const btree_superblock_t *superblock) {
        assert_thread();
        if (!dump_keys_from_delete_queue(txn, superblock->delete_queue_block, since_when_, callback_)) {
            // Set since_when_ to the minimum timestamp, so that we backfill everything.
            since_when_.time = 0;
        }
    }

    void process_a_leaf(transaction_t *txn, buf_t *leaf_node_buf) {
        assert_thread();
        const leaf_node_t *data = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        // Remember, we only want to process recent keys.

        int npairs = data->npairs;

        for (int i = 0; i < npairs; ++i) {
            uint16_t offset = data->pair_offsets[i];
            memcached_value_sizer_t sizer(txn->get_cache()->get_block_size());
            repli_timestamp_t recency = leaf::get_timestamp_value(&sizer, data, offset);
            const btree_leaf_pair *pair = leaf::get_pair(data, offset);

            if (recency.time >= since_when_.time) {
                const memcached_value_t *value = reinterpret_cast<const memcached_value_t *>(pair->value());
                boost::shared_ptr<value_data_provider_t> data_provider(value_data_provider_t::create(value, txn));
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
        int num_block_ids;
        boost::scoped_array<repli_timestamp_t> recencies;
        repli_timestamp_t since_when;

        void got_subtree_recencies() {
            coro_t::spawn(boost::bind(&annoying_t::do_got_subtree_recencies, this));
        }

        void do_got_subtree_recencies() {
            rassert(coro_t::self());
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
            interesting_children_callback_t *local_cb = cb;
            delete this;
            local_cb->receive_interesting_children(local_block_ids, num_surviving_block_ids);
        }
    };

    void filter_interesting_children(transaction_t *txn, const block_id_t *block_ids, int num_block_ids, interesting_children_callback_t *cb) {
        assert_thread();
        annoying_t *fsm = new annoying_t;
        fsm->cb = cb;
        fsm->block_ids.reset(new block_id_t[num_block_ids]);
        std::copy(block_ids, block_ids + num_block_ids, fsm->block_ids.get());
        fsm->num_block_ids = num_block_ids;
        fsm->since_when = since_when_;
        fsm->recencies.reset(new repli_timestamp_t[num_block_ids]);

        txn->get_subtree_recencies(fsm->block_ids.get(), num_block_ids, fsm->recencies.get(), fsm);
    }

    backfill_callback_t *callback_;
    repli_timestamp_t since_when_;

    backfill_traversal_helper_t(backfill_callback_t *callback, repli_timestamp_t since_when)
        : callback_(callback), since_when_(since_when) { }
};


void btree_backfill(btree_slice_t *slice, repli_timestamp_t since_when, boost::shared_ptr<cache_account_t> backfill_account, backfill_callback_t *callback, order_token_t token) {
    {
        rassert(coro_t::self());

        backfill_traversal_helper_t helper(callback, since_when);

        slice->pre_begin_transaction_sink_.check_out(token);
        order_token_t begin_transaction_token = slice->pre_begin_transaction_write_mode_source_.check_in(token.tag() + "+begin_transaction_token").with_read_mode();

        transaction_t txn(slice->cache(), rwi_read_sync);

	txn.set_token(slice->post_begin_transaction_checkpoint_.check_through(token).with_read_mode());

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

    callback->done_backfill();
}
