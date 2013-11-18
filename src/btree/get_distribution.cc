// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/get_distribution.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "btree/internal_node.hpp"

#if SLICE_ALT
using namespace alt;  // RSI:
#endif

class get_distribution_traversal_helper_t : public btree_traversal_helper_t, public home_thread_mixin_debug_only_t {
public:
    get_distribution_traversal_helper_t(int _depth_limit, std::vector<store_key_t> *_keys)
        : depth_limit(_depth_limit), key_count(0), keys(_keys)
    { }

#if SLICE_ALT
    void read_stat_block(alt_buf_lock_t *stat_block) {
#else
    void read_stat_block(buf_lock_t *stat_block) {
#endif
        if (stat_block != NULL) {
#if SLICE_ALT
            alt_buf_read_t read(stat_block);
            key_count = static_cast<const btree_statblock_t *>(read.get_data_read())->population;
#else
            key_count = static_cast<const btree_statblock_t *>(stat_block->get_data_read())->population;
#endif
        } else {
            key_count = 0;
        }
    }

    // This is free to call mark_deleted.
#if SLICE_ALT
    void process_a_leaf(alt_buf_lock_t *leaf_node_buf,
                        const btree_key_t *,
                        const btree_key_t *,
                        signal_t * /*interruptor*/,
                        int * /*population_change_out*/) THROWS_ONLY(interrupted_exc_t) {
#else
    void process_a_leaf(transaction_t *, buf_lock_t *leaf_node_buf,
                        const btree_key_t *,
                        const btree_key_t *,
                        signal_t * /*interruptor*/,
                        int * /*population_change_out*/) THROWS_ONLY(interrupted_exc_t) {
#endif
#if SLICE_ALT
        alt_buf_read_t read(leaf_node_buf);
        const leaf_node_t *node
            = static_cast<const leaf_node_t *>(read.get_data_read());
#else
        const leaf_node_t *node = static_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());
#endif

        for (auto it = leaf::begin(*node); it != leaf::end(*node); ++it) {
            const btree_key_t *key = (*it).first;
            keys->push_back(store_key_t(key->size, key->contents));
        }
    }

#if SLICE_ALT
    void postprocess_internal_node(alt_buf_lock_t *internal_node_buf) {
#else
    void postprocess_internal_node(buf_lock_t *internal_node_buf) {
#endif
#if SLICE_ALT
        alt_buf_read_t read(internal_node_buf);
        const internal_node_t *node
            = static_cast<const internal_node_t *>(read.get_data_read());
#else
        const internal_node_t *node = static_cast<const internal_node_t *>(internal_node_buf->get_data_read());
#endif

        /* Notice, we iterate all but the last pair because the last pair
         * doesn't actually have a key and we're looking for the split points.
         * */
        for (int i = 0; i < (node->npairs - 1); i++) {
            const btree_internal_pair *pair = internal_node::get_pair_by_index(node, i);
            keys->push_back(store_key_t(pair->key.size, pair->key.contents));
        }
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

#if SLICE_ALT
    alt_access_t btree_superblock_mode() {
        return alt_access_t::read;
    }
#else
    access_t btree_superblock_mode() {
        return rwi_read;
    }
#endif

#if SLICE_ALT
    alt_access_t btree_node_mode() {
        return alt_access_t::read;
    }
#else
    access_t btree_node_mode() {
        return rwi_read;
    }
#endif

    int depth_limit;
    int64_t key_count;

    //TODO this is inefficient since each one is maximum size
    std::vector<store_key_t> *keys;
};

void get_btree_key_distribution(btree_slice_t *slice,
#if !SLICE_ALT
                                transaction_t *txn,
#endif
                                superblock_t *superblock, int depth_limit,
                                int64_t *key_count_out,
                                std::vector<store_key_t> *keys_out) {
    get_distribution_traversal_helper_t helper(depth_limit, keys_out);
    rassert(keys_out->empty(), "Why is this output parameter not an empty vector\n");

    cond_t non_interruptor;
#if SLICE_ALT
    btree_parallel_traversal(superblock, slice, &helper, &non_interruptor);
#else
    btree_parallel_traversal(txn, superblock, slice, &helper, &non_interruptor);
#endif
    *key_count_out = helper.key_count;
}
