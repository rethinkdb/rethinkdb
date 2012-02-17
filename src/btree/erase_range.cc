#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "btree/erase_range.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"

class value_deleter_t {
public:
    value_deleter_t() { }
    virtual void delete_value(transaction_t *txn, void *value) = 0;

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

class erase_range_helper_t : public btree_traversal_helper_t {
public:
    erase_range_helper_t(value_sizer_t<void> *sizer, key_tester_t *tester,
                         value_deleter_t *deleter,
                         const btree_key_t *left_exclusive_or_null,
                         const btree_key_t *right_inclusive_or_null)
        : sizer_(sizer), tester_(tester), deleter_(deleter),
          left_exclusive_or_null_(left_exclusive_or_null), right_inclusive_or_null_(right_inclusive_or_null) { }

    void process_a_leaf(transaction_t *txn, buf_t *leaf_node_buf,
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

        scoped_malloc<char> value(sizer_->max_possible_size());

        for (int i = 0, e = keys_to_delete.size(); i < e; ++i) {
            if (leaf::lookup(sizer_, node, keys_to_delete[i].key(), value.get())) {
                deleter_->delete_value(txn, value.get());
            }
            leaf::erase_presence(sizer_, node, keys_to_delete[i].key(), key_modification_proof_t::fake_proof());
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
                               transaction_t *txn, got_superblock_t& superblock) {

    erase_range_helper_t helper(sizer, tester, deleter, left_exclusive_or_null, right_inclusive_or_null);
    btree_parallel_traversal(txn, superblock, slice, &helper);
}

void btree_erase_range(btree_slice_t *slice, sequence_group_t *seq_group, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       order_token_t token) {
    slice->assert_thread();

    boost::scoped_ptr<transaction_t> txn;
    got_superblock_t superblock;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.

    get_btree_superblock(slice, seq_group, rwi_write, 2, repli_timestamp_t::invalid, token, &superblock, txn);

    btree_erase_range(slice, tester, left_key_supplied, left_key_exclusive, right_key_supplied, right_key_inclusive, txn.get(), superblock);
}

void btree_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       transaction_t *txn, got_superblock_t& superblock) {

    value_sizer_t<memcached_value_t> mc_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

    struct : public value_deleter_t {
        void delete_value(transaction_t *txn, void *value) {
            blob_t blob(static_cast<memcached_value_t *>(value)->value_ref(), blob::btree_maxreflen);
            blob.clear(txn);
        }
    } deleter;

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

    btree_erase_range_generic(sizer, slice, tester, &deleter, lk, rk, txn, superblock);
}

void btree_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       const key_range_t &keys,
                       transaction_t *txn, got_superblock_t& superblock) {
    store_key_t left_exclusive(keys.left);
    store_key_t right_inclusive(keys.right.key);

    bool left_key_supplied = left_exclusive.decrement();
    bool right_key_supplied = !keys.right.unbounded;
    if (right_key_supplied) {
        right_inclusive.decrement();
    }
    btree_erase_range(slice, tester, left_key_supplied, left_exclusive, right_key_supplied, right_inclusive, txn, superblock);
}

