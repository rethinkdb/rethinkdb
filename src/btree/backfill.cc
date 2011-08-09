#include "btree/backfill.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "buffer_cache/co_functions.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/loof_node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"


class btree_slice_t;

class agnostic_backfill_callback_t {
public:
    // TODO LOOF why do some take store_key_t while others take btree_key_t.
    virtual void on_delete_range(const store_key_t& low, const store_key_t& high) = 0;
    virtual void on_deletion(const store_key_t& key, repli_timestamp_t recency) = 0;
    virtual void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const opaque_value_t *value) = 0;
    virtual void done_backfill() = 0;
    virtual ~agnostic_backfill_callback_t() { }
};

struct backfill_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_t {

    void process_a_leaf(transaction_t *txn, buf_t *leaf_node_buf) {
        assert_thread();
        const loof_t *data = reinterpret_cast<const loof_t *>(leaf_node_buf->get_data_read());

        struct : public loof::entry_reception_callback_t<void> {
            void lost_deletions() {
                // TODO LOOF call on_delete_range with appropriate range.
            }

            void deletion(const btree_key_t *k, repli_timestamp_t tstamp) {
                // TODO LOOF this is just a stupid unnecessary copy.
                store_key_t key(k->size, k->contents);
                cb->on_deletion(key, tstamp);
            }

            void key_value(const btree_key_t *k, const void *value, repli_timestamp_t tstamp) {
                cb->on_pair(txn, tstamp, k, reinterpret_cast<const opaque_value_t *>(value));
            }

            agnostic_backfill_callback_t *cb;
            transaction_t *txn;
        } x;
        x.cb = callback_;
        x.txn = txn;

        loof::dump_entries_since_time(sizer_, data, since_when_, &x);
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

    agnostic_backfill_callback_t *callback_;
    repli_timestamp_t since_when_;
    value_sizer_t<void> *sizer_;

    backfill_traversal_helper_t(agnostic_backfill_callback_t *callback, repli_timestamp_t since_when, value_sizer_t<void> *sizer)
        : callback_(callback), since_when_(since_when), sizer_(sizer) { }
};


void agnostic_btree_backfill(value_sizer_t<void> *sizer, btree_slice_t *slice, repli_timestamp_t since_when, const boost::shared_ptr<cache_account_t>& backfill_account, agnostic_backfill_callback_t *callback, order_token_t token) {
    {
        rassert(coro_t::self());

        backfill_traversal_helper_t helper(callback, since_when, sizer);

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



class agnostic_memcached_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_memcached_backfill_callback_t(backfill_callback_t *cb) : cb_(cb) { }

    void on_delete_range(const store_key_t& low, const store_key_t& high) {
        cb_->on_delete_range(low, high);
    }

    void on_deletion(const store_key_t& key, repli_timestamp_t recency) {
        cb_->on_deletion(key, recency);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const opaque_value_t *val) {
        const memcached_value_t *value = reinterpret_cast<const memcached_value_t *>(val);
        boost::shared_ptr<value_data_provider_t> data_provider(value_data_provider_t::create(value, txn));
        backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = data_provider;
        atom.flags = value->mcflags();
        atom.exptime = value->exptime();
        atom.recency = recency;
        atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
        cb_->on_keyvalue(atom);
    }

    void done_backfill() {
        cb_->done_backfill();
    }

    backfill_callback_t *cb_;
};


void btree_backfill(btree_slice_t *slice, repli_timestamp_t since_when, const boost::shared_ptr<cache_account_t>& backfill_account, backfill_callback_t *callback, order_token_t token) {
    agnostic_memcached_backfill_callback_t agnostic_cb(callback);

    value_sizer_t<memcached_value_t> sizer(slice->cache()->get_block_size());
    agnostic_btree_backfill(&sizer, slice, since_when, backfill_account, &agnostic_cb, token);
}
