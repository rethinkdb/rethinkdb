#include "btree/delete_all_keys.hpp"

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/parallel_traversal.hpp"
#include "buffer_cache/co_functions.hpp"

struct delete_all_keys_traversal_helper_t : public btree_traversal_helper_t {
    void preprocess_btree_superblock(UNUSED boost::shared_ptr<transactor_t>& txor, UNUSED const btree_superblock_t *superblock) {
        // Nothing to do here, because it's for backfill.
    }

    void process_a_leaf(boost::shared_ptr<transactor_t>& txor, buf_t *leaf_node_buf) {
        rassert(coro_t::self());
        leaf_node_t *data = reinterpret_cast<leaf_node_t *>(leaf_node_buf->get_data_major_write());

        int npairs = data->npairs;

        for (int i = 0; i < npairs; ++i) {
            uint16_t offset = data->pair_offsets[i];
            btree_leaf_pair *pair = leaf::get_pair(data, offset);

            if (pair->value()->is_large()) {
                large_buf_t lb(txor, pair->value()->lb_ref(), btree_value::lbref_limit, rwi_write);

                // TODO: We could use a callback, and not block the
                // coroutine.  (OTOH it's not as if we're blocking the
                // entire tree traversal).
                co_acquire_large_buf_for_delete(&lb);
                lb.mark_deleted();
            }
        }
    }

    void postprocess_internal_node(buf_t *internal_node_buf) {
        internal_node_buf->mark_deleted();
    }

    void postprocess_btree_superblock(buf_t *superblock_buf) {
        // We're deleting the entire tree, including the root node, so we need to do this.
        btree_superblock_t *superblock = reinterpret_cast<btree_superblock_t *>(superblock_buf->get_data_major_write());
        superblock->root_block = NULL_BLOCK_ID;
    }

    access_t transaction_mode() { return rwi_write; }
    access_t btree_superblock_mode() { return rwi_write; }
    access_t btree_node_mode() { return rwi_write; }

    void filter_interesting_children(UNUSED boost::shared_ptr<transactor_t>& txor, const block_id_t *block_ids, int num_block_ids, interesting_children_callback_t *cb) {
        // There is nothing to filter, because we want to delete everything.
        boost::scoped_array<block_id_t> ids(new block_id_t[num_block_ids]);
        std::copy(block_ids, block_ids + num_block_ids, ids.get());
        cb->receive_interesting_children(ids, num_block_ids);
    }
};

// preprocess_btree_superblock above depends on the fact that this is
// for backfill.  Thus we don't have to record the fact in the delete
// queue.  (Or, we will have to, along with the neighbor that sent us
// delete-all-keys operation.)
void btree_delete_all_keys_for_backfill(btree_slice_t *slice) {
    rassert(coro_t::self());

    delete_all_keys_traversal_helper_t helper;

    // The timestamp never gets used, because we're just deleting
    // stuff.  The use of repli_timestamp::invalid here might trip
    // some assertions, though.
    btree_parallel_traversal(slice, repli_timestamp::invalid, &helper);
}
