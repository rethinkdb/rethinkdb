// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/erase_range.hpp"

#include "buffer_cache/alt.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/types.hpp"
#include "concurrency/fifo_checker.hpp"

class erase_range_helper_t : public btree_traversal_helper_t {
public:
    erase_range_helper_t(value_sizer_t *sizer, key_tester_t *tester,
            const value_deleter_t *deleter, const btree_key_t *left_exclusive_or_null,
            const btree_key_t *right_inclusive_or_null,
            const std::function<void(const store_key_t &, const char *,
                                     const buf_parent_t &)>
                &on_erase_cb)
        : sizer_(sizer), tester_(tester), deleter_(deleter),
          left_exclusive_or_null_(left_exclusive_or_null),
          right_inclusive_or_null_(right_inclusive_or_null),
          on_erase_cb_(on_erase_cb)
    { }

    void process_a_leaf(buf_lock_t *leaf_node_buf,
                        const btree_key_t *l_excl,
                        const btree_key_t *r_incl,
                        signal_t *,
                        int *population_change_out) THROWS_ONLY(interrupted_exc_t) {
        buf_write_t write(leaf_node_buf);
        leaf_node_t *node = static_cast<leaf_node_t *>(write.get_data_write());

        std::vector<store_key_t> keys_to_delete;

        for (auto it = leaf::begin(*node); it != leaf::end(*node); ++it) {
            const btree_key_t *k = (*it).first;
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

        int population_change = 0;

        for (size_t i = 0; i < keys_to_delete.size(); ++i) {
            bool found = leaf::lookup(sizer_, node, keys_to_delete[i].btree_key(),
                                      value.get());
            guarantee(found);

            if (on_erase_cb_) {
                on_erase_cb_(keys_to_delete[i], value.get(), buf_parent_t(leaf_node_buf));
            }

            deleter_->delete_value(buf_parent_t(leaf_node_buf), value.get());
            leaf::erase_presence(sizer_, node, keys_to_delete[i].btree_key(),
                                 key_modification_proof_t::real_proof());
            --population_change;
        }

        *population_change_out = population_change;
    }

    void postprocess_internal_node(UNUSED buf_lock_t *internal_node_buf) {
        // We don't want to do anything here.
    }

    void filter_interesting_children(buf_parent_t,
                                     ranged_block_ids_t *ids_source,
                                     interesting_children_callback_t *cb) {
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

    access_t btree_superblock_mode() { return access_t::write; }
    access_t btree_node_mode() { return access_t::write; }

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
    value_sizer_t *sizer_;
    key_tester_t *tester_;
    const value_deleter_t *deleter_;
    const btree_key_t *left_exclusive_or_null_;
    const btree_key_t *right_inclusive_or_null_;
    std::function<void(const store_key_t &, const char *, const buf_parent_t &)>
        on_erase_cb_;

    DISABLE_COPYING(erase_range_helper_t);
};

void btree_erase_range_generic(value_sizer_t *sizer,
        key_tester_t *tester, const value_deleter_t *deleter,
        const btree_key_t *left_exclusive_or_null,
        const btree_key_t *right_inclusive_or_null,
        superblock_t *superblock, signal_t *interruptor,
        release_superblock_t release_superblock,
        const std::function<void(const store_key_t &, const char *, const buf_parent_t &)>
            &on_erase_cb) {
    erase_range_helper_t helper(sizer, tester, deleter,
                                left_exclusive_or_null, right_inclusive_or_null,
                                on_erase_cb);
    btree_parallel_traversal(superblock, &helper, interruptor,
                             release_superblock);
}
