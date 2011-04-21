#include "btree/backfill.hpp"

#include <algorithm>

#include "errors.hpp"
#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/transactor.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/parallel_traversal.hpp"

class btree_slice_t;

struct backfill_traversal_helper_t : public btree_traversal_helper_t {

    // The deletes have to go first (since they get overridden by
    // newer sets)
    void preprocess_btree_superblock(boost::shared_ptr<transactor_t>& txor, const btree_superblock_t *superblock) {
        dump_keys_from_delete_queue(txor, superblock->delete_queue_block, since_when_, callback_);
    }

    void process_a_leaf(boost::shared_ptr<transactor_t>& txor, buf_t *leaf_node_buf) {
        const leaf_node_t *data = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        // Remember, we only want to process recent keys.

        int npairs = data->npairs;

        for (int i = 0; i < npairs; ++i) {
            uint16_t offset = data->pair_offsets[i];
            repli_timestamp recency = leaf::get_timestamp_value(data, offset);
            const btree_leaf_pair *pair = leaf::get_pair(data, offset);

            if (recency.time >= since_when_.time) {
                const btree_value *value = pair->value();
                unique_ptr_t<value_data_provider_t> data_provider(value_data_provider_t::create(value, txor));
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
    access_t btree_superblock_mode() { return rwi_read; }
    access_t btree_node_mode() { return rwi_read; }

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

    void filter_interesting_children(boost::shared_ptr<transactor_t>& txor, const block_id_t *block_ids, int num_block_ids, interesting_children_callback_t *cb) {
        annoying_t *fsm = new annoying_t;
        fsm->cb = cb;
        fsm->block_ids.reset(new block_id_t[num_block_ids]);
        std::copy(block_ids, block_ids + num_block_ids, fsm->block_ids.get());
        fsm->num_block_ids = num_block_ids;
        fsm->since_when = since_when_;
        fsm->recencies.reset(new repli_timestamp[num_block_ids]);

        txor->get()->get_subtree_recencies(fsm->block_ids.get(), num_block_ids, fsm->recencies.get(), fsm);
    }

    backfill_callback_t *callback_;
    repli_timestamp since_when_;

    backfill_traversal_helper_t(backfill_callback_t *callback, repli_timestamp since_when)
        : callback_(callback), since_when_(since_when) { }
};



void btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback) {
    rassert(coro_t::self());

    backfill_traversal_helper_t helper(callback, since_when);

    btree_parallel_traversal(slice, repli_timestamp::invalid, &helper);

    callback->done();
}
