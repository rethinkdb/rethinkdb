#include "btree/get_distribution.hpp"
#include "btree/parallel_traversal.hpp"
#include "utils.hpp"

class get_distribution_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_t {
public:
    get_distribution_traversal_helper_t(int _depth_limit, std::vector<store_key_t> *_keys)
        : depth_limit(_depth_limit), key_count(0), keys(_keys)
    { }

    void read_stat_block(buf_lock_t *stat_block) { 
        key_count = reinterpret_cast<const btree_statblock_t *>(stat_block->get_data_read())->population;
    }

    // This is free to call mark_deleted.
    void process_a_leaf(transaction_t *, buf_lock_t *leaf_node_buf,
                                const btree_key_t *,
                                const btree_key_t *,
                                int *) {
        const leaf_node_t *node = reinterpret_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        leaf::live_iter_t it = iter_for_whole_leaf(node);

        const btree_key_t *key;
        while ((key = it.get_key(node))) {
            keys->push_back(store_key_t(key->size, key->contents));
            it.step(node);
        }
    }

    void postprocess_internal_node(buf_lock_t *internal_node_buf) {
        const internal_node_t *node = reinterpret_cast<const internal_node_t *>(internal_node_buf->get_data_read());

        for (int i = 0; i < node->npairs; i++) {
            const btree_internal_pair *pair = internal_node::get_pair_by_index(node, i);
            keys->push_back(store_key_t(pair->key.size, pair->key.contents));
        }
    }

    void postprocess_btree_superblock(buf_lock_t *) {
    }

    void filter_interesting_children(transaction_t *, ranged_block_ids_t *ids_source, interesting_children_callback_t *cb) {
        if (ids_source->get_level() < depth_limit) {
            int num_block_ids = ids_source->num_block_ids();
            for (int i = 0; i < num_block_ids; ++i) {
                block_id_t block_id;
                const btree_key_t *left, *right;
                ids_source->get_block_id_and_bounding_interval(i, &block_id, &left, &right);

                cb->receive_interesting_child(i);
            }
        } else {
            //We're over the depth limit and thus disinterested in all children
        }

        cb->no_more_interesting_children();
    }

    access_t btree_superblock_mode() {
        return rwi_read;
    }

    access_t btree_node_mode() {
        return rwi_read;
    }

    int depth_limit;
    int key_count;

    //TODO this is inefficient since each one is maximum size
    std::vector<store_key_t> *keys; 
};

void get_btree_key_distribution(btree_slice_t *slice, transaction_t *txn, got_superblock_t& superblock, int depth_limit, int *key_count_out, std::vector<store_key_t> *keys_out) {
    get_distribution_traversal_helper_t helper(depth_limit, keys_out);
    rassert(keys_out->empty(), "Why is this output paremeter not an empty vector\n");

    btree_parallel_traversal(txn, superblock, slice, &helper);
    *key_count_out = helper.key_count;
}
