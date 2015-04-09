// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/erase_range.hpp"

#include "buffer_cache/alt.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/reql_specific.hpp"
#include "btree/types.hpp"
#include "concurrency/promise.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/store.hpp"

class collect_keys_helper_t : public depth_first_traversal_callback_t {
public:
    collect_keys_helper_t(key_tester_t *tester,
                          const key_range_t &key_range,
                          uint64_t max_keys_to_collect, /* 0 = unlimited */
                          signal_t *interruptor)
        : aborted_(false),
          tester_(tester),
          key_range_(key_range),
          max_keys_to_collect_(max_keys_to_collect),
          interruptor_(interruptor) {
        if (max_keys_to_collect_ != 0) {
            collected_keys_.reserve(max_keys_to_collect_);
        }
    }

    done_traversing_t handle_pair(scoped_key_value_t &&keyvalue) {
        guarantee(!aborted_);
        guarantee(key_range_.contains_key(
            keyvalue.key()->contents, keyvalue.key()->size));
        if (!tester_->key_should_be_erased(keyvalue.key())) {
            return done_traversing_t::NO;
        }
        store_key_t key(keyvalue.key());
        collected_keys_.push_back(key);
        if (collected_keys_.size() == max_keys_to_collect_ ||
                interruptor_->is_pulsed()) {
            aborted_ = true;
            return done_traversing_t::YES;
        } else {
            return done_traversing_t::NO;
        }
    }

    const std::vector<store_key_t> &get_collected_keys() const {
        return collected_keys_;
    }

    bool get_aborted() const {
        return aborted_;
    }

private:
    std::vector<store_key_t> collected_keys_;
    bool aborted_;

    key_tester_t *tester_;
    key_range_t key_range_;
    uint64_t max_keys_to_collect_;
    signal_t *interruptor_;

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
        std::vector<rdb_modification_report_t> *mod_reports_out,
        key_range_t *deleted_out) {
    rassert(mod_reports_out != nullptr);
    rassert(deleted_out != nullptr);
    mod_reports_out->clear();
    *deleted_out = key_range_t::empty();

    /* Step 1: Collect all keys that we want to erase using a depth-first traversal. */
    collect_keys_helper_t key_collector(tester, key_range, max_keys_to_erase,
        interruptor);
    btree_depth_first_traversal(superblock, key_range, &key_collector,
                                direction_t::FORWARD, release_superblock_t::KEEP);
    if (interruptor->is_pulsed()) {
        /* If the interruptor is pulsed during the depth-first traversal, then the
        traversal will stop early but not throw an exception. So we have to throw it
        here. */
        throw interrupted_exc_t();
    }

    /* Step 2: Erase each key individually and create the corresponding
       modification reports. */
    const max_block_size_t max_block_size = superblock->cache()->max_block_size();
    rdb_value_sizer_t sizer(max_block_size);
    for (const auto &key : key_collector.get_collected_keys()) {
        promise_t<superblock_t *> pass_back_superblock_promise;
        {
            keyvalue_location_t kv_location;
            find_keyvalue_location_for_write(
                &sizer,
                superblock,
                key.btree_key(),
                /* don't update subtree recencies as we traverse the tree */
                repli_timestamp_t::distant_past,
                deletion_context->balancing_detacher(),
                &kv_location,
                NULL /* profile::trace_t */,
                &pass_back_superblock_promise);
            btree_slice->stats.pm_keys_set.record();
            btree_slice->stats.pm_total_keys_set += 1;

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

        guarantee(key >= deleted_out->right.key);
        *deleted_out = key_range_t(key_range_t::closed, key_range.left,
                                   key_range_t::closed, key);

        if (interruptor->is_pulsed()) {
            /* Note: We have to check the interruptor at the beginning or the end of the
            loop. If we check it in the middle, we might leave the B-tree in a half-
            consistent state, or we might leave `*deleted_out` in a state not consistent
            with what we actually deleted. */
            throw interrupted_exc_t();
        }
    }

    /* If we're done, then set `*deleted_out` to be exactly the same as the range we were
    supposed to delete. This isn't redundant because there may be a gap between the last
    key we actually deleted and the true right-hand side of `key_range`. */
    if (!key_collector.get_aborted()) {
        *deleted_out = key_range;
    }

    return key_collector.get_aborted() ? done_traversing_t::NO : done_traversing_t::YES;
}

