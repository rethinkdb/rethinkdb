// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/operations.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/promise.hpp"
#include "rdb_protocol/profile.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

/* Passing in a pass_back_superblock parameter will cause this function to
 * return the superblock after it's no longer needed (rather than releasing
 * it). Notice the superblock is not guaranteed to be returned until the
 * keyvalue_location_t that's passed in (keyvalue_location_out) is destroyed.
 * This is because it may need to use the superblock for some of its methods.
 * */
#if SLICE_ALT
// RSI: It seems like really we should pass the superblock_t via rvalue reference.
// Is that possible?  (promise_t makes it hard.)
template <class Value>
void find_keyvalue_location_for_write(
        superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        btree_stats_t *stats,
        profile::trace_t *trace,
        promise_t<superblock_t *> *pass_back_superblock = NULL) {
#else
template <class Value>
void find_keyvalue_location_for_write(
        transaction_t *txn, superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        eviction_priority_t *root_eviction_priority, btree_stats_t *stats,
        profile::trace_t *trace,
        promise_t<superblock_t *> *pass_back_superblock = NULL) {
#endif
#if SLICE_ALT
    value_sizer_t<Value> sizer(superblock->expose_buf().cache()->max_block_size());
#else
    value_sizer_t<Value> sizer(txn->get_cache()->get_block_size());
#endif

    keyvalue_location_out->superblock = superblock;
    keyvalue_location_out->pass_back_superblock = pass_back_superblock;

#if SLICE_ALT
    ensure_stat_block(superblock);
#else
    ensure_stat_block(txn, superblock, incr_priority(ZERO_EVICTION_PRIORITY));
#endif
    keyvalue_location_out->stat_block = keyvalue_location_out->superblock->get_stat_block_id();

    keyvalue_location_out->stats = stats;

#if SLICE_ALT
    // RSI: Make sure we do the logic smart here -- don't needlessly hold both
    // buffers.  (This finds the keyvalue for _write_ so that probably won't really
    // happen.)
    alt::alt_buf_lock_t last_buf;
    alt::alt_buf_lock_t buf;
#else
    buf_lock_t last_buf;
    buf_lock_t buf;
#endif
    {
        profile::starter_t starter("Acquiring block for write.\n", trace);
#if SLICE_ALT
        get_root(&sizer, superblock, &buf);
#else
        get_root(&sizer, txn, superblock, &buf, *root_eviction_priority);
#endif
    }

    // Walk down the tree to the leaf.
#if SLICE_ALT
    for (;;) {
        {
            alt::alt_buf_read_t read(&buf);
            if (!node::is_internal(static_cast<const node_t *>(read.get_data_read()))) {
                break;
            }
        }
#else
    while (node::is_internal(static_cast<const node_t *>(buf.get_data_read()))) {
#endif
        // Check if the node is overfull and proactively split it if it is (since this is an internal node).
        {
            profile::starter_t starter("Perhaps split node.", trace);
#if SLICE_ALT
            // RSI: Ugh, we're passing the superblock, buf, and last_buf.  Really?
            check_and_handle_split(&sizer, &buf, &last_buf, superblock, key, static_cast<Value *>(NULL));
#else
            check_and_handle_split(&sizer, txn, &buf, &last_buf, superblock, key, static_cast<Value *>(NULL), root_eviction_priority);
#endif
        }

        // Check if the node is underfull, and merge/level if it is.
        {
            profile::starter_t starter("Perhaps merge nodes.", trace);
#if SLICE_ALT
            // RSI: Ugh, we're passing the superblock, buf, last_buf here too.
            check_and_handle_underfull(&sizer, &buf, &last_buf, superblock, key);
#else
            check_and_handle_underfull(&sizer, txn, &buf, &last_buf, superblock, key);
#endif
        }

        // Release the superblock, if we've gone past the root (and haven't
        // already released it). If we're still at the root or at one of
        // its direct children, we might still want to replace the root, so
        // we can't release the superblock yet.
#if SLICE_ALT
        if (!last_buf.empty() && keyvalue_location_out->superblock) {
#else
        if (last_buf.is_acquired() && keyvalue_location_out->superblock) {
#endif
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
#if SLICE_ALT
        last_buf.reset_buf_lock();
#endif

        // Look up and acquire the next node.
#if SLICE_ALT
        block_id_t node_id;
        {
            alt::alt_buf_read_t read(&buf);
            auto node = static_cast<const internal_node_t *>(read.get_data_read());
            node_id = internal_node::lookup(node, key);
        }
#else
        block_id_t node_id = internal_node::lookup(static_cast<const internal_node_t *>(buf.get_data_read()), key);
#endif
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            profile::starter_t starter("Acquiring block for write.\n", trace);
#if SLICE_ALT
            alt::alt_buf_lock_t tmp(&buf, node_id, alt::alt_access_t::write);
            last_buf = std::move(buf);
            buf = std::move(tmp);
#else
            buf_lock_t tmp(txn, node_id, rwi_write);
            tmp.set_eviction_priority(incr_priority(buf.get_eviction_priority()));
            last_buf.swap(tmp);
            buf.swap(last_buf);
#endif
        }
    }

    {
        scoped_malloc_t<Value> tmp(sizer.max_possible_size());

        // We've gone down the tree and gotten to a leaf. Now look up the key.
#if SLICE_ALT
        alt::alt_buf_read_t read(&buf);
        auto node = static_cast<const leaf_node_t *>(read.get_data_read());
        bool key_found = leaf::lookup(&sizer, node, key, tmp.get());
#else
        bool key_found = leaf::lookup(&sizer, static_cast<const leaf_node_t *>(buf.get_data_read()), key, tmp.get());
#endif

        if (key_found) {
            keyvalue_location_out->there_originally_was_value = true;
            keyvalue_location_out->value = std::move(tmp);
        }
    }

    // RSI: keyvalue_location_out really saves last_buf?  Why??
    keyvalue_location_out->last_buf.swap(last_buf);
    keyvalue_location_out->buf.swap(buf);
}

#if SLICE_ALT
template <class Value>
void find_keyvalue_location_for_read(
        superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        btree_stats_t *stats, profile::trace_t *trace) {
#else
template <class Value>
void find_keyvalue_location_for_read(
        transaction_t *txn, superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t<Value> *keyvalue_location_out,
        eviction_priority_t root_eviction_priority, btree_stats_t *stats,
        profile::trace_t *trace) {
#endif
    stats->pm_keys_read.record();
#if SLICE_ALT
    value_sizer_t<Value> sizer(superblock->expose_buf().cache()->max_block_size());
#else
    value_sizer_t<Value> sizer(txn->get_cache()->get_block_size());
#endif

    const block_id_t root_id = superblock->get_root_block_id();
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        // There is no root, so the tree is empty.
        superblock->release();
        return;
    }

#if SLICE_ALT
    alt::alt_buf_lock_t buf;
#else
    buf_lock_t buf;
#endif
    {
        profile::starter_t starter("Acquire a block for read.", trace);
#if SLICE_ALT
        alt::alt_buf_lock_t tmp(superblock->expose_buf(), root_id,
                                alt::alt_access_t::read);
        superblock->release();
        buf = std::move(tmp);
#else
        buf_lock_t tmp(txn, root_id, rwi_read);
        tmp.set_eviction_priority(root_eviction_priority);
        buf.swap(tmp);
#endif
    }

#if !SLICE_ALT
    superblock->release();
#endif

#ifndef NDEBUG
#if SLICE_ALT
    {
        alt::alt_buf_read_t read(&buf);
        node::validate(&sizer, static_cast<const node_t *>(read.get_data_read()));
    }
#else
    node::validate(&sizer, static_cast<const node_t *>(buf.get_data_read()));
#endif
#endif  // NDEBUG

#if SLICE_ALT
    for (;;) {
        {
            alt::alt_buf_read_t read(&buf);
            if (!node::is_internal(static_cast<const node_t *>(read.get_data_read()))) {
                break;
            }
        }
#else
    while (node::is_internal(static_cast<const node_t *>(buf.get_data_read()))) {
#endif
#if SLICE_ALT
        block_id_t node_id;
        {
            alt::alt_buf_read_t read(&buf);
            const internal_node_t *node
                = static_cast<const internal_node_t *>(read.get_data_read());
            node_id = internal_node::lookup(node, key);
        }
#else
        const block_id_t node_id = internal_node::lookup(static_cast<const internal_node_t *>(buf.get_data_read()), key);
#endif
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            profile::starter_t starter("Acquire a block for read.", trace);
#if SLICE_ALT
            alt::alt_buf_lock_t tmp(&buf, node_id, alt::alt_access_t::read);
            buf.reset_buf_lock();
            buf = std::move(tmp);
#else
            buf_lock_t tmp(txn, node_id, rwi_read);
            tmp.set_eviction_priority(incr_priority(buf.get_eviction_priority()));
            buf.swap(tmp);
#endif
        }

#ifndef NDEBUG
#if SLICE_ALT
        {
            alt::alt_buf_read_t read(&buf);
            node::validate(&sizer, static_cast<const node_t *>(read.get_data_read()));
        }
#else
        node::validate(&sizer, static_cast<const node_t *>(buf.get_data_read()));
#endif
#endif  // NDEBUG
    }

    // Got down to the leaf, now probe it.
#if SLICE_ALT
    scoped_malloc_t<Value> value(sizer.max_possible_size());
    bool value_found;
    {
        alt::alt_buf_read_t read(&buf);
        const leaf_node_t *leaf
            = static_cast<const leaf_node_t *>(read.get_data_read());
        value_found = leaf::lookup(&sizer, leaf, key, value.get());
    }
#else
    const leaf_node_t *leaf = static_cast<const leaf_node_t *>(buf.get_data_read());
    scoped_malloc_t<Value> value(sizer.max_possible_size());
    const bool value_found = leaf::lookup(&sizer, leaf, key, value.get());
#endif
    if (value_found) {
#if SLICE_ALT
        keyvalue_location_out->buf = std::move(buf);
#else
        keyvalue_location_out->buf.swap(buf);
#endif
        keyvalue_location_out->there_originally_was_value = true;
        keyvalue_location_out->value = std::move(value);
    }
}

#if SLICE_ALT
template <class Value>
void apply_keyvalue_change(keyvalue_location_t<Value> *kv_loc,
                           const btree_key_t *key, repli_timestamp_t tstamp,
                           bool expired,
                           key_modification_callback_t<Value> *km_callback) {
#else
template <class Value>
void apply_keyvalue_change(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, const btree_key_t *key, repli_timestamp_t tstamp, bool expired, key_modification_callback_t<Value> *km_callback, eviction_priority_t *root_eviction_priority) {
#endif

#if SLICE_ALT
    value_sizer_t<Value> sizer(kv_loc->buf.cache()->get_block_size());
#else
    value_sizer_t<Value> sizer(txn->get_cache()->get_block_size());
#endif

#if SLICE_ALT
    key_modification_proof_t km_proof
        = km_callback->value_modification(kv_loc, key);
#else
    key_modification_proof_t km_proof
        = km_callback->value_modification(txn, kv_loc, key);
#endif

    /* how much this keyvalue change affects the total population of the btree
     * (should be -1, 0 or 1) */
    int population_change;

    if (kv_loc->value.has()) {
        // We have a value to insert.

        // Split the node if necessary, to make sure that we have room
        // for the value.  Not necessary when deleting, because the
        // node won't grow.

#if SLICE_ALT
        check_and_handle_split(&sizer, &kv_loc->buf, &kv_loc->last_buf,
                               kv_loc->superblock, key, kv_loc->value.get());
#else
        check_and_handle_split(&sizer, txn, &kv_loc->buf, &kv_loc->last_buf, kv_loc->superblock, key, kv_loc->value.get(), root_eviction_priority);
#endif

#if SLICE_ALT
        {
            alt::alt_buf_read_t read(&kv_loc->buf);
            auto leaf_node = static_cast<const leaf_node_t *>(read.get_data_read());
            rassert(!leaf::is_full(&sizer, leaf_node, key, kv_loc->value.get()));
        }
#else
        rassert(!leaf::is_full(&sizer, static_cast<const leaf_node_t *>(kv_loc->buf.get_data_read()),
                key, kv_loc->value.get()));
#endif

        if (kv_loc->there_originally_was_value) {
            population_change = 0;
        } else {
            population_change = 1;
        }

        {
#if SLICE_ALT
            alt::alt_buf_write_t write(&kv_loc->buf);
            auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
#else
            auto leaf_node = static_cast<leaf_node_t *>(kv_loc->buf.get_data_write());
#endif
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
            if (!expired) {
                rassert(tstamp != repli_timestamp_t::invalid, "Deletes need a valid timestamp now.");
                {
#if SLICE_ALT
                    alt::alt_buf_write_t write(&kv_loc->buf);
                    auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
#else
                    auto leaf_node = static_cast<leaf_node_t *>(kv_loc->buf.get_data_write());
#endif
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
#if SLICE_ALT
                    alt::alt_buf_write_t write(&kv_loc->buf);
                    auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
#else
                    auto leaf_node = static_cast<leaf_node_t *>(kv_loc->buf.get_data_write());
#endif
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
#if SLICE_ALT
    check_and_handle_underfull(&sizer, &kv_loc->buf, &kv_loc->last_buf,
                               kv_loc->superblock, key);
#else
    check_and_handle_underfull(&sizer, txn, &kv_loc->buf, &kv_loc->last_buf, kv_loc->superblock, key);
#endif

    //Modify the stats block
#if SLICE_ALT
    // RSI: Should we _actually_ pass kv_loc->buf as the parent?
    // RSI: This was opened with some buffer_cache_order_mode_ignore flag.
    alt::alt_buf_lock_t stat_block(&kv_loc->buf, kv_loc->stat_block,
                              alt::alt_access_t::write);
    alt::alt_buf_write_t stat_block_write(&stat_block);
    auto stat_block_buf
        = static_cast<btree_statblock_t *>(stat_block_write.get_data_write());
    stat_block_buf->population += population_change;
#else
    buf_lock_t stat_block(txn, kv_loc->stat_block, rwi_write, buffer_cache_order_mode_ignore);
    static_cast<btree_statblock_t *>(stat_block.get_data_write())->population += population_change;
#endif
}

template <class Value>
void apply_keyvalue_change(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, btree_key_t *key, repli_timestamp_t tstamp, key_modification_callback_t<Value> *km_callback, eviction_priority_t *root_eviction_priority) {
    apply_keyvalue_change(txn, kv_loc, key, tstamp, false, km_callback, root_eviction_priority);
}

