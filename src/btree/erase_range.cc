// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/erase_range.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"

class erase_range_helper_t : public btree_traversal_helper_t {
public:
    erase_range_helper_t(value_sizer_t<void> *sizer, key_tester_t *tester,
                         value_deleter_t *deleter,
                         const btree_key_t *left_exclusive_or_null,
                         const btree_key_t *right_inclusive_or_null)
        : sizer_(sizer), tester_(tester), deleter_(deleter),
          left_exclusive_or_null_(left_exclusive_or_null),
          right_inclusive_or_null_(right_inclusive_or_null)
    { }

    void process_a_leaf(transaction_t *txn, buf_lock_t *leaf_node_buf,
                        const btree_key_t *l_excl,
                        const btree_key_t *r_incl,
                        signal_t *,
                        int *population_change_out) THROWS_ONLY(interrupted_exc_t) {
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(leaf_node_buf->get_data_write());

        std::vector<store_key_t> keys_to_delete;

        for (leaf::live_iter_t iter = leaf::iter_for_whole_leaf(node); /* no test */; iter.step(node)) {
            const btree_key_t *k = iter.get_key(node);
            if (!k) {
                break;
            }

            // k's in the leaf node so it should be in the range of
            // keys allowed for the leaf node.
            assert_key_in_range(k, l_excl, r_incl);

            if (key_in_range(k, left_exclusive_or_null_, right_inclusive_or_null_) && tester_->key_should_be_erased(k)) {
                keys_to_delete.push_back(store_key_t(k));
            }
        }

        scoped_malloc_t<char> value(sizer_->max_possible_size());

        for (size_t i = 0; i < keys_to_delete.size(); ++i) {
            bool found = leaf::lookup(sizer_, node, keys_to_delete[i].btree_key(), value.get());
            guarantee(found);
            deleter_->delete_value(txn, value.get());
            leaf::erase_presence(sizer_, node, keys_to_delete[i].btree_key(),
                                 key_modification_proof_t::real_proof());
        }

        *population_change_out = -static_cast<int>(keys_to_delete.size());
    }

    void postprocess_internal_node(UNUSED buf_lock_t *internal_node_buf) {
        // We don't want to do anything here.
    }

    void filter_interesting_children(UNUSED transaction_t *txn, ranged_block_ids_t *ids_source, interesting_children_callback_t *cb) {
        for (int i = 0, e = ids_source->num_block_ids(); i < e; ++i) {
            block_id_t block_id;
            const btree_key_t *left, *right;
            ids_source->get_block_id_and_bounding_interval(i, &block_id, &left, &right);

            if (overlaps(left, right, left_exclusive_or_null_, right_inclusive_or_null_)) {
                cb->receive_interesting_child(i);
            }
        }

        cb->no_more_interesting_children();
    }

    access_t btree_superblock_mode() { return rwi_write; }
    access_t btree_node_mode() { return rwi_write; }

    ~erase_range_helper_t() { }

    static bool key_in_range(const btree_key_t *k, const btree_key_t *left_excl, const btree_key_t *right_incl) {
        if (left_excl != NULL && sized_strcmp(k->contents, k->size, left_excl->contents, left_excl->size) <= 0) {
            return false;
        }
        if (right_incl != NULL && sized_strcmp(right_incl->contents, right_incl->size, k->contents, k->size) < 0) {
            return false;
        }
        return true;
    }

    static void assert_key_in_range(DEBUG_VAR const btree_key_t *k, DEBUG_VAR const btree_key_t *left_excl, DEBUG_VAR const btree_key_t *right_incl) {
        rassert(key_in_range(k, left_excl, right_incl));
    }

    // Checks if (x_l_excl, x_r_incl] intersects (y_l_excl, y_r_incl].
    static bool overlaps(const btree_key_t *x_l_excl, const btree_key_t *x_r_incl,
                         const btree_key_t *y_l_excl, const btree_key_t *y_r_incl) {
        return (x_l_excl == NULL || y_r_incl == NULL || sized_strcmp(x_l_excl->contents, x_l_excl->size, y_r_incl->contents, y_r_incl->size) < 0)
            && (x_r_incl == NULL || y_l_excl == NULL || sized_strcmp(y_l_excl->contents, y_l_excl->size, x_r_incl->contents, x_r_incl->size) < 0);
    }

private:
    value_sizer_t<void> *sizer_;
    key_tester_t *tester_;
    value_deleter_t *deleter_;
    const btree_key_t *left_exclusive_or_null_;
    const btree_key_t *right_inclusive_or_null_;

    DISABLE_COPYING(erase_range_helper_t);
};

void btree_erase_range_generic(value_sizer_t<void> *sizer, btree_slice_t *slice,
                               key_tester_t *tester,
                               value_deleter_t *deleter,
                               const btree_key_t *left_exclusive_or_null,
                               const btree_key_t *right_inclusive_or_null,
                               transaction_t *txn, superblock_t *superblock,
                               signal_t *interruptor,
                               bool release_superblock) {
    erase_range_helper_t helper(sizer, tester, deleter, left_exclusive_or_null, right_inclusive_or_null);
    btree_parallel_traversal(txn, superblock, slice, &helper, interruptor, release_superblock);
}

void erase_all(value_sizer_t<void> *sizer, btree_slice_t *slice,
               value_deleter_t *deleter, transaction_t *txn,
               superblock_t *superblock,
               signal_t *interruptor,
               bool release_superblock) {
    struct always_true_tester_t : public key_tester_t {
        bool key_should_be_erased(const btree_key_t *) { return true; }
    } always_true_tester;

    btree_erase_range_generic(sizer, slice, &always_true_tester,
                              deleter, NULL, NULL, txn, superblock,
                              interruptor, release_superblock);
}
