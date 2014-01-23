// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/operations.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "concurrency/promise.hpp"
#include "rdb_protocol/profile.hpp"

// TODO: consider B#/B* trees to improve space efficiency

/* Passing in a pass_back_superblock parameter will cause this function to
 * return the superblock after it's no longer needed (rather than releasing
 * it). Notice the superblock is not guaranteed to be returned until the
 * keyvalue_location_t that's passed in (keyvalue_location_out) is destroyed.
 * This is because it may need to use the superblock for some of its methods.
 * */
// KSI: It seems like really we should pass the superblock_t via rvalue reference.
// Is that possible?  (promise_t makes it hard.)
template <class Value>
void find_keyvalue_location_for_write(
        superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        btree_stats_t *stats,
        profile::trace_t *trace,
        promise_t<superblock_t *> *pass_back_superblock = NULL) {
    value_sizer_t<Value> sizer(superblock->expose_buf().cache()->max_block_size());

    keyvalue_location_out->superblock = superblock;
    keyvalue_location_out->pass_back_superblock = pass_back_superblock;

    ensure_stat_block(superblock);
    keyvalue_location_out->stat_block = keyvalue_location_out->superblock->get_stat_block_id();

    keyvalue_location_out->stats = stats;

    // RSI: Make sure we do the logic smart here -- don't needlessly hold both
    // buffers.  (This finds the keyvalue for _write_ so that probably won't really
    // happen.)
    buf_lock_t last_buf;
    buf_lock_t buf;
    {
        // RSI: We can't acquire the block for write here -- we could, but it would
        // worsen the performance of the program -- sometimes we only end up using
        // this block for read.  So the profiling information is not very good.
        profile::starter_t starter("Acquiring block for write.\n", trace);
        buf = get_root(&sizer, superblock);
    }

    // Walk down the tree to the leaf.
    for (;;) {
        {
            buf_read_t read(&buf);
            if (!node::is_internal(static_cast<const node_t *>(read.get_data_read()))) {
                break;
            }
        }
        // Check if the node is overfull and proactively split it if it is (since this is an internal node).
        {
            profile::starter_t starter("Perhaps split node.", trace);
            // RSI: Ugh, we're passing the superblock, buf, and last_buf.  Really?
            check_and_handle_split(&sizer, &buf, &last_buf, superblock, key, static_cast<Value *>(NULL));
        }

        // Check if the node is underfull, and merge/level if it is.
        {
            profile::starter_t starter("Perhaps merge nodes.", trace);
            // RSI: Ugh, we're passing the superblock, buf, last_buf here too.
            check_and_handle_underfull(&sizer, &buf, &last_buf, superblock, key);
        }

        // Release the superblock, if we've gone past the root (and haven't
        // already released it). If we're still at the root or at one of
        // its direct children, we might still want to replace the root, so
        // we can't release the superblock yet.
        if (!last_buf.empty() && keyvalue_location_out->superblock) {
            if (pass_back_superblock != NULL) {
                pass_back_superblock->pulse(superblock);
                keyvalue_location_out->superblock = NULL;
            } else {
                keyvalue_location_out->superblock->release();
                keyvalue_location_out->superblock = NULL;
            }
        }

        // Release the old previous node (unless we're at the root), and set
        // the next previous node (which is the current node).
        last_buf.reset_buf_lock();

        // Look up and acquire the next node.
        block_id_t node_id;
        {
            buf_read_t read(&buf);
            auto node = static_cast<const internal_node_t *>(read.get_data_read());
            node_id = internal_node::lookup(node, key);
        }
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            profile::starter_t starter("Acquiring block for write.\n", trace);
            buf_lock_t tmp(&buf, node_id, alt_access_t::write);
            last_buf = std::move(buf);
            buf = std::move(tmp);
        }
    }

    {
        scoped_malloc_t<Value> tmp(sizer.max_possible_size());

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        buf_read_t read(&buf);
        auto node = static_cast<const leaf_node_t *>(read.get_data_read());
        bool key_found = leaf::lookup(&sizer, node, key, tmp.get());

        if (key_found) {
            keyvalue_location_out->there_originally_was_value = true;
            keyvalue_location_out->value = std::move(tmp);
        }
    }

    // RSI: keyvalue_location_out really saves last_buf?  Why??
    keyvalue_location_out->last_buf.swap(last_buf);
    keyvalue_location_out->buf.swap(buf);
}

template <class Value>
void find_keyvalue_location_for_read(
        superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        btree_stats_t *stats, profile::trace_t *trace) {
    stats->pm_keys_read.record();
    value_sizer_t<Value> sizer(superblock->expose_buf().cache()->max_block_size());

    const block_id_t root_id = superblock->get_root_block_id();
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        // There is no root, so the tree is empty.
        superblock->release();
        return;
    }

    buf_lock_t buf;
    {
        profile::starter_t starter("Acquire a block for read.", trace);
        buf_lock_t tmp(superblock->expose_buf(), root_id, alt_access_t::read);
        superblock->release();
        buf = std::move(tmp);
    }

#ifndef NDEBUG
    {
        buf_read_t read(&buf);
        node::validate(&sizer, static_cast<const node_t *>(read.get_data_read()));
    }
#endif  // NDEBUG

    for (;;) {
        {
            buf_read_t read(&buf);
            if (!node::is_internal(static_cast<const node_t *>(read.get_data_read()))) {
                break;
            }
        }
        block_id_t node_id;
        {
            buf_read_t read(&buf);
            const internal_node_t *node
                = static_cast<const internal_node_t *>(read.get_data_read());
            node_id = internal_node::lookup(node, key);
        }
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            profile::starter_t starter("Acquire a block for read.", trace);
            buf_lock_t tmp(&buf, node_id, alt_access_t::read);
            buf.reset_buf_lock();
            buf = std::move(tmp);
        }

#ifndef NDEBUG
        {
            buf_read_t read(&buf);
            node::validate(&sizer, static_cast<const node_t *>(read.get_data_read()));
        }
#endif  // NDEBUG
    }

    // Got down to the leaf, now probe it.
    scoped_malloc_t<Value> value(sizer.max_possible_size());
    bool value_found;
    {
        buf_read_t read(&buf);
        const leaf_node_t *leaf
            = static_cast<const leaf_node_t *>(read.get_data_read());
        value_found = leaf::lookup(&sizer, leaf, key, value.get());
    }
    if (value_found) {
        keyvalue_location_out->buf = std::move(buf);
        keyvalue_location_out->there_originally_was_value = true;
        keyvalue_location_out->value = std::move(value);
    }
}

enum class expired_t { NO, YES };

template <class Value>
void apply_keyvalue_change(keyvalue_location_t<Value> *kv_loc,
        const btree_key_t *key, repli_timestamp_t tstamp, expired_t expired,
        key_modification_callback_t<Value> *km_callback) {

    value_sizer_t<Value> sizer(kv_loc->buf.cache()->get_block_size());

    key_modification_proof_t km_proof
        = km_callback->value_modification(kv_loc, key);

    /* how much this keyvalue change affects the total population of the btree
     * (should be -1, 0 or 1) */
    int population_change;

    if (kv_loc->value.has()) {
        // We have a value to insert.

        // Split the node if necessary, to make sure that we have room
        // for the value.  Not necessary when deleting, because the
        // node won't grow.

        check_and_handle_split(&sizer, &kv_loc->buf, &kv_loc->last_buf,
                               kv_loc->superblock, key, kv_loc->value.get());

        {
#ifndef NDEBUG
            buf_read_t read(&kv_loc->buf);
            auto leaf_node = static_cast<const leaf_node_t *>(read.get_data_read());
            rassert(!leaf::is_full(&sizer, leaf_node, key, kv_loc->value.get()));
#endif
        }

        if (kv_loc->there_originally_was_value) {
            population_change = 0;
        } else {
            population_change = 1;
        }

        {
            buf_write_t write(&kv_loc->buf);
            auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
            leaf::insert(&sizer,
                         leaf_node,
                         key,
                         kv_loc->value.get(),
                         tstamp,
                         km_proof);
        }

        kv_loc->stats->pm_keys_set.record();
    } else {
        // Delete the value if it's there.
        if (kv_loc->there_originally_was_value) {
            if (expired == expired_t::NO) {
                rassert(tstamp != repli_timestamp_t::invalid, "Deletes need a valid timestamp now.");
                {
                    buf_write_t write(&kv_loc->buf);
                    auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
                    leaf::remove(&sizer,
                                 leaf_node,
                                 key,
                                 tstamp,
                                 km_proof);
                }
                population_change = -1;
                kv_loc->stats->pm_keys_set.record();
            } else {
                // TODO: Oh god oh god get rid of "expired".
                // Expirations do an erase, not a delete.
                {
                    buf_write_t write(&kv_loc->buf);
                    auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
                    leaf::erase_presence(&sizer,
                                         leaf_node,
                                         key,
                                         km_proof);
                }
                population_change = 0;
                kv_loc->stats->pm_keys_expired.record();
            }
        } else {
            population_change = 0;
        }
    }

    // Check to see if the leaf is underfull (following a change in
    // size or a deletion, and merge/level if it is.
    check_and_handle_underfull(&sizer, &kv_loc->buf, &kv_loc->last_buf,
                               kv_loc->superblock, key);

    // Modify the stats block.
    // RSI: Should we _actually_ pass kv_loc->buf as the parent?
    // RSI: See parallel_traversal.cc for another use of the stat block -- we do the
    // same thing there.
    buf_lock_t stat_block(&kv_loc->buf, kv_loc->stat_block, alt_access_t::write);
    buf_write_t stat_block_write(&stat_block);
    auto stat_block_buf
        = static_cast<btree_statblock_t *>(stat_block_write.get_data_write());
    stat_block_buf->population += population_change;
}
