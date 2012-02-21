#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node_functions.hpp"
#include "btree/slice.hpp"
#include "btree/buf_patches.hpp"
#include "buffer_cache/buffer_cache.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.


inline void insert_root(block_id_t root_id, superblock_t* sb) {
    sb->set_root_block_id(root_id);
    sb->release();
}

// Get a root block given a superblock, or make a new root if there isn't one.
template <class Value>
void get_root(value_sizer_t<Value> *sizer, transaction_t *txn, superblock_t* sb, buf_lock_t *buf_out, eviction_priority_t root_eviction_priority) {
    rassert(!buf_out->is_acquired());
    rassert(ZERO_EVICTION_PRIORITY < root_eviction_priority);

    block_id_t node_id = sb->get_root_block_id();

    if (node_id != NULL_BLOCK_ID) {
        buf_lock_t temp_lock(txn, node_id, rwi_write);
        buf_out->swap(temp_lock);
    } else {
        buf_lock_t temp_lock(txn);
        buf_out->swap(temp_lock);
        leaf::init(sizer, reinterpret_cast<leaf_node_t *>(buf_out->get_data_major_write()));
        insert_root(buf_out->get_block_id(), sb);
    }

    if (buf_out->get_eviction_priority() == DEFAULT_EVICTION_PRIORITY) {
        buf_out->set_eviction_priority(root_eviction_priority);
    }
}


// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
template <class Value>
void check_and_handle_split(value_sizer_t<Value> *sizer, transaction_t *txn, buf_lock_t& buf, buf_lock_t& last_buf, superblock_t *sb,
                            const btree_key_t *key, void *new_value, eviction_priority_t *root_eviction_priority) {
    txn->assert_thread();

    const node_t *node = reinterpret_cast<const node_t *>(buf.get_data_read());

    // If the node isn't full, we don't need to split, so we're done.
    if (!node::is_internal(node)) { // This should only be called when update_needed.
        rassert(new_value);
        if (!leaf::is_full(sizer, reinterpret_cast<const leaf_node_t *>(node), key, new_value)) {
            return;
        }
    } else {
        rassert(!new_value);
        if (!internal_node::is_full(reinterpret_cast<const internal_node_t *>(node))) {
            return;
        }
    }

    // Allocate a new node to split into, and some temporary memory to keep
    // track of the median key in the split; then actually split.
    buf_lock_t rbuf(txn);
    btree_key_buffer_t median_buffer;
    btree_key_t *median = median_buffer.key();

    node::split(sizer, &buf, reinterpret_cast<node_t *>(rbuf.get_data_major_write()), median);
    rbuf.set_eviction_priority(buf.get_eviction_priority());

    // Insert the key that sets the two nodes apart into the parent.
    if (!last_buf.is_acquired()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        buf_lock_t temp_buf(txn);
        last_buf.swap(temp_buf);
        internal_node::init(sizer->block_size(), reinterpret_cast<internal_node_t *>(last_buf.get_data_major_write()));
        rassert(ZERO_EVICTION_PRIORITY < buf.get_eviction_priority());
        last_buf.set_eviction_priority(decr_priority(buf.get_eviction_priority()));
        *root_eviction_priority = last_buf.get_eviction_priority();

        insert_root(last_buf.get_block_id(), sb);
    }

    bool success UNUSED = internal_node::insert(sizer->block_size(), &last_buf, median, buf.get_block_id(), rbuf.get_block_id());
    rassert(success, "could not insert internal btree node");

    // We've split the node; now figure out where the key goes and release the other buf (since we're done with it).
    if (0 >= sized_strcmp(key->contents, key->size, median->contents, median->size)) {
        // The key goes in the old buf (the left one).

        // Do nothing.

    } else {
        // The key goes in the new buf (the right one).
        buf.swap(rbuf);
    }
}

// Merge or level the node if necessary.
template <class Value>
void check_and_handle_underfull(value_sizer_t<Value> *sizer, transaction_t *txn,
                                buf_lock_t& buf, buf_lock_t& last_buf, superblock_t *sb,
                                const btree_key_t *key) {
    const node_t *node = reinterpret_cast<const node_t *>(buf.get_data_read());
    if (last_buf.is_acquired() && node::is_underfull(sizer, node)) { // The root node is never underfull.

        const internal_node_t *parent_node = reinterpret_cast<const internal_node_t *>(last_buf.get_data_read());

        // Acquire a sibling to merge or level with.
        btree_key_buffer_t key_in_middle;
        block_id_t sib_node_id;
        int nodecmp_node_with_sib = internal_node::sibling(parent_node, key, &sib_node_id, &key_in_middle);

        // Now decide whether to merge or level.
        buf_lock_t sib_buf(txn, sib_node_id, rwi_write);
        const node_t *sib_node = reinterpret_cast<const node_t *>(sib_buf.get_data_read());

#ifndef NDEBUG
        node::validate(sizer, sib_node);
#endif

        if (node::is_mergable(sizer, node, sib_node, parent_node)) { // Merge.

            if (nodecmp_node_with_sib < 0) { // Nodes must be passed to merge in ascending order.
                node::merge(sizer, const_cast<node_t *>(node), &sib_buf, parent_node);
                buf.mark_deleted();
                buf.swap(sib_buf);
            } else {
                node::merge(sizer, const_cast<node_t *>(sib_node), &buf, parent_node);
                sib_buf.mark_deleted();
            }

            sib_buf.release();

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(sizer->block_size(), &last_buf, key_in_middle.key());
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf.mark_deleted();
                insert_root(buf.get_block_id(), sb);
            }
        } else { // Level
            btree_key_buffer_t replacement_key_buffer;
            btree_key_t *replacement_key = replacement_key_buffer.key();

            bool leveled = node::level(sizer, nodecmp_node_with_sib, &buf, &sib_buf, replacement_key, parent_node);

            if (leveled) {
                internal_node::update_key(&last_buf, key_in_middle.key(), replacement_key);
            }
        }
    }
}

inline void get_btree_superblock(btree_slice_t *slice, sequence_group_t *seq_group, access_t access, int expected_change_count, repli_timestamp_t tstamp, order_token_t token, got_superblock_t *got_superblock_out, boost::scoped_ptr<transaction_t>& txn_out) {
    slice->assert_thread();

    slice->pre_begin_transaction_sink_.check_out(token);
    order_token_t begin_transaction_token = (is_read_mode(access) ? slice->pre_begin_transaction_read_mode_source_ : slice->pre_begin_transaction_write_mode_source_).check_in(token.tag() + "+begin_transaction_token");
    if (is_read_mode(access)) {
        begin_transaction_token = begin_transaction_token.with_read_mode();
    }
    txn_out.reset(new transaction_t(slice->cache(), seq_group, access, expected_change_count, tstamp));
    txn_out->set_token(slice->post_begin_transaction_checkpoint_.check_through(begin_transaction_token));

    buf_lock_t tmp_buf(txn_out.get(), SUPERBLOCK_ID, access);
    boost::scoped_ptr<superblock_t> tmp_sb(new real_superblock_t(tmp_buf));
    tmp_sb->set_eviction_priority(ZERO_EVICTION_PRIORITY);
    got_superblock_out->sb.swap(tmp_sb);
}

inline void get_btree_superblock(btree_slice_t *slice, sequence_group_t *seq_group, access_t access, order_token_t token, got_superblock_t *got_superblock_out, boost::scoped_ptr<transaction_t>& txn_out) {
    rassert(is_read_mode(access));
    get_btree_superblock(slice, seq_group, access, 0, repli_timestamp_t::distant_past, token, got_superblock_out, txn_out);
}

template <class Value>
void find_keyvalue_location_for_write(transaction_t *txn, got_superblock_t *got_superblock, btree_key_t *key, keyvalue_location_t<Value> *keyvalue_location_out, eviction_priority_t *root_eviction_priority) {
    keyvalue_location_out->sb.swap(got_superblock->sb);
    value_sizer_t<Value> v_sizer(txn->get_cache()->get_block_size());
    value_sizer_t<void> *sizer = &v_sizer;

    buf_lock_t last_buf;
    buf_lock_t buf;
    get_root(sizer, txn, keyvalue_location_out->sb.get(), &buf, *root_eviction_priority);

    // Walk down the tree to the leaf.
    while (node::is_internal(reinterpret_cast<const node_t *>(buf.get_data_read()))) {
        // Check if the node is overfull and proactively split it if it is (since this is an internal node).
        check_and_handle_split(sizer, txn, buf, last_buf, keyvalue_location_out->sb.get(), key, reinterpret_cast<Value *>(NULL), root_eviction_priority);

        // Check if the node is underfull, and merge/level if it is.
        check_and_handle_underfull(sizer, txn, buf, last_buf, keyvalue_location_out->sb.get(), key);

        // Release the superblock, if we've gone past the root (and haven't
        // already released it). If we're still at the root or at one of
        // its direct children, we might still want to replace the root, so
        // we can't release the superblock yet.
        if (last_buf.is_acquired()) {
            keyvalue_location_out->sb->release();
        }

        // Release the old previous node (unless we're at the root), and set
        // the next previous node (which is the current node).

        // Look up and acquire the next node.
        block_id_t node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf.get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        buf_lock_t tmp(txn, node_id, rwi_write);
        tmp.set_eviction_priority(incr_priority(buf.get_eviction_priority()));
        last_buf.swap(tmp);
        buf.swap(last_buf);
    }

    {
        scoped_malloc<Value> tmp(sizer->max_possible_size());

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        bool key_found = leaf::lookup(sizer, reinterpret_cast<const leaf_node_t *>(buf.get_data_read()), key, tmp.get());

        if (key_found) {
            keyvalue_location_out->there_originally_was_value = true;
            keyvalue_location_out->value.swap(tmp);
        }
    }

    keyvalue_location_out->last_buf.swap(last_buf);
    keyvalue_location_out->buf.swap(buf);
}

template <class Value>
void find_keyvalue_location_for_read(transaction_t *txn, got_superblock_t *got_superblock, btree_key_t *key, keyvalue_location_t<Value> *keyvalue_location_out, eviction_priority_t root_eviction_priority) {
    block_id_t node_id = got_superblock->sb->get_root_block_id();
    rassert(node_id != SUPERBLOCK_ID);

    buf_lock_t buf;
    got_superblock->sb->swap_buf(buf);

    value_sizer_t<Value> v_sizer(txn->get_cache()->get_block_size());
    value_sizer_t<void> *sizer = &v_sizer;

    if (node_id == NULL_BLOCK_ID) {
        // There is no root, so the tree is empty.
        return;
    }

    {
        buf_lock_t tmp(txn, node_id, rwi_read);
        tmp.set_eviction_priority(root_eviction_priority);
        buf.swap(tmp);
    }

#ifndef NDEBUG
    node::validate(sizer, reinterpret_cast<const node_t *>(buf.get_data_read()));
#endif  // NDEBUG

    while (node::is_internal(reinterpret_cast<const node_t *>(buf.get_data_read()))) {
        node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf.get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            buf_lock_t tmp(txn, node_id, rwi_read);
            tmp.set_eviction_priority(incr_priority(buf.get_eviction_priority()));
            buf.swap(tmp);
        }

#ifndef NDEBUG
        node::validate(sizer, reinterpret_cast<const node_t *>(buf.get_data_read()));
#endif  // NDEBUG
    }

    // Got down to the leaf, now probe it.
    const leaf_node_t *leaf = reinterpret_cast<const leaf_node_t *>(buf.get_data_read());
    scoped_malloc<Value> value(sizer->max_possible_size());
    if(leaf::lookup(sizer, leaf, key, value.get())) {
        keyvalue_location_out->buf.swap(buf);
        keyvalue_location_out->there_originally_was_value = true;
        keyvalue_location_out->value.swap(value);
    }
}

template <class Value>
void apply_keyvalue_change(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, btree_key_t *key, repli_timestamp_t tstamp, bool expired, key_modification_callback_t<Value> *km_callback, eviction_priority_t *root_eviction_priority) {
    value_sizer_t<Value> v_sizer(txn->get_cache()->get_block_size());
    value_sizer_t<void> *sizer = &v_sizer;

    key_modification_proof_t km_proof = km_callback->value_modification(txn, kv_loc, key);

    if (kv_loc->value) {
        // We have a value to insert.

        // Split the node if necessary, to make sure that we have room
        // for the value.  Not necessary when deleting, because the
        // node won't grow.

        check_and_handle_split(sizer, txn, kv_loc->buf, kv_loc->last_buf, kv_loc->sb.get(), key, kv_loc->value.get(), root_eviction_priority);

        rassert(!leaf::is_full(sizer, reinterpret_cast<const leaf_node_t *>(kv_loc->buf.get_data_read()),
                key, kv_loc->value.get()));

        leaf_patched_insert(sizer, &kv_loc->buf, key, kv_loc->value.get(), tstamp, km_proof);
    } else {
        // Delete the value if it's there.
        if (kv_loc->there_originally_was_value) {
            if (!expired) {
                rassert(tstamp != repli_timestamp_t::invalid, "Deletes need a valid timestamp now.");
                leaf_patched_remove(&kv_loc->buf, key, tstamp, km_proof);
            } else {
                // Expirations do an erase, not a delete.
                leaf_patched_erase_presence(&kv_loc->buf, key, km_proof);
            }
        }
    }

    // Check to see if the leaf is underfull (following a change in
    // size or a deletion, and merge/level if it is.
    check_and_handle_underfull(sizer, txn, kv_loc->buf, kv_loc->last_buf, kv_loc->sb.get(), key);
}

template <class Value>
void apply_keyvalue_change(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, btree_key_t *key, repli_timestamp_t tstamp, key_modification_callback_t<Value> *km_callback, eviction_priority_t *root_eviction_priority) {
    apply_keyvalue_change(txn, kv_loc, key, tstamp, false, km_callback, root_eviction_priority);
}

template <class Value>
value_txn_t<Value>::value_txn_t(btree_key_t *_key,
                                keyvalue_location_t<Value>& _kv_location,
                                repli_timestamp_t _tstamp,
                                key_modification_callback_t<Value> *_km_callback,
                                eviction_priority_t *_root_eviction_priority)
    : key(_key), tstamp(_tstamp), root_eviction_priority(_root_eviction_priority), km_callback(_km_callback)
{
    kv_location.swap(_kv_location);
}

template <class Value>
value_txn_t<Value>::value_txn_t(btree_slice_t *slice, sequence_group_t *seq_group, btree_key_t *_key, const repli_timestamp_t _tstamp, const order_token_t token,
                                key_modification_callback_t<Value> *_km_callback)
    : key(_key), tstamp(_tstamp), root_eviction_priority(&slice->root_eviction_priority), km_callback(_km_callback)
{
    got_superblock_t can_haz_superblock;

    get_btree_superblock(slice, seq_group, rwi_write, 1, tstamp, token, &can_haz_superblock, txn);

    keyvalue_location_t<Value> _kv_location;
    find_keyvalue_location_for_write(txn.get(), &can_haz_superblock, key, &_kv_location, &slice->root_eviction_priority);

    kv_location.swap(_kv_location);
}

template <class Value>
value_txn_t<Value>::~value_txn_t() {
    apply_keyvalue_change(txn.get(), &kv_location, key, tstamp, false, km_callback, root_eviction_priority);
}

template <class Value>
scoped_malloc<Value>& value_txn_t<Value>::value() {
    return kv_location.value;
}

template <class Value>
transaction_t *value_txn_t<Value>::get_txn() {
    return txn.get();
}

template <class Value>
void get_value_read(btree_slice_t *slice, sequence_group_t *seq_group, btree_key_t *key, order_token_t token, keyvalue_location_t<Value> *kv_location_out, boost::scoped_ptr<transaction_t>& txn_out) {
    got_superblock_t got_superblock;
    get_btree_superblock(slice, seq_group, rwi_read, token, &got_superblock, txn_out);

    find_keyvalue_location_for_read(txn_out.get(), &got_superblock, key, kv_location_out, slice->root_eviction_priority);
}
