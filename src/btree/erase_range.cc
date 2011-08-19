#include "btree/erase_range.hpp"

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"
#include "errors.hpp"

class erase_range_helper_t : public btree_traversal_helper_t {
public:
    erase_range_helper_t(value_sizer_t<void> *sizer, key_tester_t *tester,
                         const btree_key_t *left_exclusive_or_null,
                         const btree_key_t *right_inclusive_or_null)
        : sizer_(sizer), tester_(tester),
          left_exclusive_or_null_(left_exclusive_or_null), right_inclusive_or_null_(right_inclusive_or_null) { }

    void process_a_leaf(UNUSED transaction_t *txn, buf_t *leaf_node_buf,
                        UNUSED const btree_key_t *l_excl,
                        UNUSED const btree_key_t *r_incl) {
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(leaf_node_buf->get_data_major_write());

        std::vector<btree_key_buffer_t> keys_to_delete;


        for (leaf::live_iter_t iter = leaf::iter_for_whole_leaf(node); /* no test */; iter.step(node)) {
            const btree_key_t *k = iter.get_key(node);
            if (!k) {
                break;
            }

            // k's in the leaf node so it should be in the range of
            // keys allowed for the leaf node.
            rassert(key_in_range(k, l_excl, r_incl));

            if (key_in_range(k, left_exclusive_or_null_, right_inclusive_or_null_) && tester_->key_should_be_erased(k)) {
                keys_to_delete.push_back(btree_key_buffer_t(k));
            }
        }

        for (int i = 0, e = keys_to_delete.size(); i < e; ++i) {
            leaf::erase_presence(sizer_, node, keys_to_delete[i].key());
        }
    }

    void postprocess_internal_node(UNUSED buf_t *internal_node_buf) {
        // We don't want to do anything here.
    }

    void postprocess_btree_superblock(UNUSED buf_t *superblock_buf) {
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

    // Checks if (x_l_excl, x_r_incl] intersects (y_l_excl, y_r_incl].
    static bool overlaps(const btree_key_t *x_l_excl, const btree_key_t *x_r_incl,
                         const btree_key_t *y_l_excl, const btree_key_t *y_r_incl) {
        return (x_l_excl == NULL || sized_strcmp(x_l_excl->contents, x_l_excl->size, y_r_incl->contents, y_r_incl->size) < 0)
            && (x_r_incl == NULL || sized_strcmp(y_l_excl->contents, y_l_excl->size, x_r_incl->contents, x_r_incl->size) < 0);
    }

private:
    value_sizer_t<void> *sizer_;
    key_tester_t *tester_;
    const btree_key_t *left_exclusive_or_null_;
    const btree_key_t *right_inclusive_or_null_;

    DISABLE_COPYING(erase_range_helper_t);
};


void btree_erase_range_generic(value_sizer_t<void> *sizer, btree_slice_t *slice,
                               key_tester_t *tester,
                               const btree_key_t *left_exclusive_or_null,
                               const btree_key_t *right_inclusive_or_null,
                               order_token_t token) {
    slice->assert_thread();

    slice->pre_begin_transaction_sink_.check_out(token);
    order_token_t begin_transaction_token = slice->pre_begin_transaction_write_mode_source_.check_in(token.tag() + "+begin_transaction_token");

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.

    transaction_t txn(slice->cache(), rwi_write, 2, repli_timestamp_t::invalid);
    txn.set_token(slice->post_begin_transaction_checkpoint_.check_through(begin_transaction_token));

    erase_range_helper_t helper(sizer, tester, left_exclusive_or_null, right_inclusive_or_null);

    btree_parallel_traversal(&txn, slice, &helper);
}

void btree_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       order_token_t token) {
    value_sizer_t<memcached_value_t> mc_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

    // TODO: Sigh, stupid wasteful copies.
    btree_key_buffer_t left, right;
    btree_key_t *lk = NULL, *rk = NULL;
    if (left_key_supplied) {
        left.assign(left_key_exclusive.contents, left_key_exclusive.contents + left_key_exclusive.size);
        lk = left.key();
    }
    if (right_key_supplied) {
        const char *p = right_key_inclusive.contents;
        right.assign(p, p + right_key_inclusive.size);
        rk = right.key();
    }

    btree_erase_range_generic(sizer, slice, tester, lk, rk, token);
}
