// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/erase_range.hpp"

#include "buffer_cache/alt.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/types.hpp"
#include "concurrency/promise.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/store.hpp"

/* Helper function for rdb_erase_small_range() */
void rdb_erase_range_convert_keys(const key_range_t &key_range,
                                  bool *left_key_supplied_out,
                                  bool *right_key_supplied_out,
                                  store_key_t *left_key_exclusive_out,
                                  store_key_t *right_key_inclusive_out) {
    /* This is guaranteed because the way the keys are calculated below would
     * lead to a single key being deleted even if the range was empty. */
    guarantee(!key_range.is_empty());

    rassert(left_key_supplied_out != NULL);
    rassert(right_key_supplied_out != NULL);
    rassert(left_key_exclusive_out != NULL);
    rassert(right_key_inclusive_out != NULL);

    /* Twiddle some keys to get the in the form we want. Notice these are keys
     * which will be made  exclusive and inclusive as their names suggest
     * below. At the point of construction they aren't. */
    *left_key_exclusive_out = store_key_t(key_range.left);
    *right_key_inclusive_out = store_key_t(key_range.right.key);

    *left_key_supplied_out = left_key_exclusive_out->decrement();
    *right_key_supplied_out = !key_range.right.unbounded;
    if (*right_key_supplied_out) {
        right_key_inclusive_out->decrement();
    }

    /* Now left_key_exclusive and right_key_inclusive accurately reflect their
     * names. */
}

class collect_keys_helper_t : public btree_traversal_helper_t {
public:
    collect_keys_helper_t(key_tester_t *tester,
                          const btree_key_t *left_exclusive_or_null,
                          const btree_key_t *right_inclusive_or_null,
                          uint64_t max_keys_to_collect /* 0 = unlimited */)
        : tester_(tester),
          left_exclusive_or_null_(left_exclusive_or_null),
          right_inclusive_or_null_(right_inclusive_or_null),
          max_keys_to_collect_(max_keys_to_collect) {
        if (max_keys_to_collect_ != 0) {
            collected_keys_.reserve(max_keys_to_collect_);
        }
        // Will be set to NO later if we abort prematurely.
        done_traversing_ = done_traversing_t::YES;
    }

    void process_a_leaf(buf_lock_t *leaf_node_buf,
                        const btree_key_t *l_excl,
                        const btree_key_t *r_incl,
                        signal_t *,
                        int *) THROWS_ONLY(interrupted_exc_t) {
        buf_read_t read(leaf_node_buf);
        const leaf_node_t *node = static_cast<const leaf_node_t *>(read.get_data_read());

        for (auto it = leaf::begin(*node); it != leaf::end(*node); ++it) {
            if (max_keys_to_collect_ != 0
                && collected_keys_.size() >= max_keys_to_collect_) {

                rassert(collected_keys_.size() == max_keys_to_collect_);
                // There might be more overlapping key/value pairs, but we're
                // aborting now. Set done_traversing_ to NO.
                done_traversing_ = done_traversing_t::NO;
                break;
            }

            const btree_key_t *k = (*it).first;
            if (!k) {
                break;
            }

            // k's in the leaf node so it should be in the range of
            // keys allowed for the leaf node.
            assert_key_in_range(k, l_excl, r_incl);

            if (key_in_range(k, left_exclusive_or_null_, right_inclusive_or_null_)
                && tester_->key_should_be_erased(k)) {
                collected_keys_.push_back(store_key_t(k));
            }
        }
    }

    void postprocess_internal_node(UNUSED buf_lock_t *internal_node_buf) {
        // We don't want to do anything here.
    }

    void filter_interesting_children(buf_parent_t,
                                     ranged_block_ids_t *ids_source,
                                     interesting_children_callback_t *cb) {
        for (int i = 0, e = ids_source->num_block_ids(); i < e; ++i) {
            if (max_keys_to_collect_ != 0
                && collected_keys_.size() >= max_keys_to_collect_) {

                rassert(collected_keys_.size() == max_keys_to_collect_);
                // There might be more overlapping children, but we're aborting
                // now. Set done_traversing_ to NO.
                done_traversing_ = done_traversing_t::NO;
                break;
            }

            block_id_t block_id;
            const btree_key_t *left, *right;
            ids_source->get_block_id_and_bounding_interval(i, &block_id, &left, &right);

            if (overlaps(left, right, left_exclusive_or_null_, right_inclusive_or_null_)) {
                cb->receive_interesting_child(i);
            }
        }

        cb->no_more_interesting_children();
    }

    access_t btree_superblock_mode() { return access_t::read; }
    access_t btree_node_mode() { return access_t::read; }

    const std::vector<store_key_t> &get_collected_keys() const {
        return collected_keys_;
    }

    done_traversing_t get_done_traversing() const {
        return done_traversing_;
    }

private:
    static bool key_in_range(const btree_key_t *k,
                             const btree_key_t *left_excl,
                             const btree_key_t *right_incl) {
        if (left_excl != NULL && btree_key_cmp(k, left_excl) <= 0) {
            return false;
        }
        if (right_incl != NULL && btree_key_cmp(right_incl, k) < 0) {
            return false;
        }
        return true;
    }

    static void assert_key_in_range(DEBUG_VAR const btree_key_t *k,
                                    DEBUG_VAR const btree_key_t *left_excl,
                                    DEBUG_VAR const btree_key_t *right_incl) {
        rassert(key_in_range(k, left_excl, right_incl));
    }

    // Checks if (x_l_excl, x_r_incl] intersects (y_l_excl, y_r_incl].
    static bool overlaps(const btree_key_t *x_l_excl, const btree_key_t *x_r_incl,
                         const btree_key_t *y_l_excl, const btree_key_t *y_r_incl) {
        return (x_l_excl == NULL || y_r_incl == NULL
                || btree_key_cmp(x_l_excl, y_r_incl) < 0)
            && (x_r_incl == NULL || y_l_excl == NULL
                || btree_key_cmp(y_l_excl, x_r_incl) < 0);
    }

    std::vector<store_key_t> collected_keys_;
    done_traversing_t done_traversing_;

    key_tester_t *tester_;
    const btree_key_t *left_exclusive_or_null_;
    const btree_key_t *right_inclusive_or_null_;
    uint64_t max_keys_to_collect_;

    DISABLE_COPYING(collect_keys_helper_t);
};

done_traversing_t rdb_erase_small_range(
        btree_slice_t *btree_slice,
        key_tester_t *tester,
        const key_range_t &key_range,
        superblock_t *superblock,
        const deletion_context_t *deletion_context,
        signal_t *interruptor,
        uint64_t max_keys_to_erase,
        std::vector<rdb_modification_report_t> *mod_reports_out) {
    rassert(mod_reports_out != NULL);
    mod_reports_out->clear();

    bool left_key_supplied, right_key_supplied;
    store_key_t left_key_exclusive, right_key_inclusive;
    rdb_erase_range_convert_keys(key_range, &left_key_supplied, &right_key_supplied,
             &left_key_exclusive, &right_key_inclusive);

    /* Step 1: Collect all keys that we want to erase using a parallel traversal. */
    collect_keys_helper_t key_collector(
        tester,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        max_keys_to_erase);
    btree_parallel_traversal(superblock, &key_collector, interruptor,
                             release_superblock_t::KEEP);

    /* Step 2: Erase each key individually and create the corresponding
       modification reports. */
    const max_block_size_t max_block_size = superblock->cache()->max_block_size();
    rdb_value_sizer_t sizer(max_block_size);
    for (const auto &key : key_collector.get_collected_keys()) {
        promise_t<superblock_t *> pass_back_superblock_promise;
        {
            keyvalue_location_t kv_location;
            find_keyvalue_location_for_write(&sizer, superblock, key.btree_key(),
                                             deletion_context->balancing_detacher(),
                                             &kv_location,
                                             &btree_slice->stats,
                                             NULL /* profile::trace_t */,
                                             &pass_back_superblock_promise);

            // We're still holding a write lock on the superblock, so if the value
            // disappeared since we've populated key_collector, something fishy
            // is going on.
            guarantee(kv_location.value.has());

            // The mod_report we generate is a simple delete. While there is generally
            // a difference between an erase and a delete (deletes get backfilled,
            // while an erase is as if the value had never existed), that
            // difference is irrelevant in the case of secondary indexes.
            rdb_modification_report_t mod_report;
            mod_report.primary_key = key;
            // Get the full data
            const rdb_value_t *rdb_value = kv_location.value_as<rdb_value_t>();
            mod_report.info.deleted.first = get_data(rdb_value,
                                                     buf_parent_t(&kv_location.buf));
            // Get the inline value
            mod_report.info.deleted.second.assign(rdb_value->value_ref(),
                rdb_value->value_ref() + rdb_value->inline_size(max_block_size));
            mod_reports_out->push_back(mod_report);

            // Detach the value
            deletion_context->in_tree_deleter()->delete_value(
                buf_parent_t(&kv_location.buf), kv_location.value.get());
            // Erase the entry from the leaf node
            kv_location.value.reset();
            null_key_modification_callback_t null_cb;
            apply_keyvalue_change(&sizer, &kv_location, key.btree_key(),
                                  repli_timestamp_t::invalid /* ignored for erase */,
                                  deletion_context->in_tree_deleter(),
                                  &null_cb,
                                  delete_or_erase_t::ERASE);
        } // kv_location is destroyed here. That's important because sometimes
          // pass_back_superblock_promise isn't pulsed before the kv_location
          // gets deleted.
        guarantee(pass_back_superblock_promise.wait() == superblock);
    }

    return key_collector.get_done_traversing();
}

