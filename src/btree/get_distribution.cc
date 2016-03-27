// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/get_distribution.hpp"

#include "btree/internal_node.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/parallel_traversal.hpp"
#include "buffer_cache/alt.hpp"
#include "utils.hpp"

class get_distribution_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_debug_only_t {
public:
    get_distribution_traversal_helper_t(int _depth_limit, std::vector<store_key_t> *_keys)
        : depth_limit(_depth_limit), key_count(0), keys(_keys)
    { }

    void read_stat_block(buf_lock_t *stat_block) {
        guarantee (stat_block != nullptr);
        buf_read_t read(stat_block);
        uint32_t sb_size;
        const btree_statblock_t *sb_data =
            static_cast<const btree_statblock_t *>(read.get_data_read(&sb_size));
        guarantee(sb_size == BTREE_STATBLOCK_SIZE);
        key_count = sb_data->population;
    }

    // This is free to call mark_deleted.
    void process_a_leaf(buf_lock_t *leaf_node_buf,
                        const btree_key_t *,
                        const btree_key_t *,
                        signal_t * /*interruptor*/,
                        int * /*population_change_out*/) THROWS_ONLY(interrupted_exc_t) {
        buf_read_t read(leaf_node_buf);
        const leaf_node_t *node
            = static_cast<const leaf_node_t *>(read.get_data_read());

        for (auto it = leaf::begin(*node); it != leaf::end(*node); ++it) {
            const btree_key_t *key = (*it).first;
            keys->push_back(store_key_t(key->size, key->contents));
        }
    }

    void postprocess_internal_node(buf_lock_t *internal_node_buf) {
        buf_read_t read(internal_node_buf);
        const internal_node_t *node
            = static_cast<const internal_node_t *>(read.get_data_read());

        /* Notice, we iterate all but the last pair because the last pair
         * doesn't actually have a key and we're looking for the split points.
         * */
        for (int i = 0; i < (node->npairs - 1); i++) {
            const btree_internal_pair *pair = internal_node::get_pair_by_index(node, i);
            keys->push_back(store_key_t(pair->key.size, pair->key.contents));
        }
    }

    void filter_interesting_children(buf_parent_t,
                                     ranged_block_ids_t *ids_source,
                                     interesting_children_callback_t *cb) {
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
        return access_t::read;
    }

    access_t btree_node_mode() {
        return access_t::read;
    }

    int depth_limit;
    int64_t key_count;

    //TODO this is inefficient since each one is maximum size
    std::vector<store_key_t> *keys;
};

void get_btree_key_distribution(superblock_t *superblock, int depth_limit,
                                int64_t *key_count_out,
                                std::vector<store_key_t> *keys_out) {
    get_distribution_traversal_helper_t helper(depth_limit, keys_out);
    rassert(keys_out->empty(), "Why is this output parameter not an empty vector\n");

    cond_t non_interruptor;
    btree_parallel_traversal(superblock, &helper, &non_interruptor);
    *key_count_out = helper.key_count;
}
