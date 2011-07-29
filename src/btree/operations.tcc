#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/node_functions.hpp"
#include "btree/slice.hpp"


// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.



inline void insert_root(block_id_t root_id, buf_lock_t& sb_buf) {
    rassert(sb_buf.is_acquired());
    // TODO: WTF is up with this const cast?  This makes NO SENSE.  gtfo.
    sb_buf->set_data(const_cast<block_id_t *>(&reinterpret_cast<const btree_superblock_t *>(sb_buf->get_data_read())->root_block), &root_id, sizeof(root_id));

    sb_buf.release();
}

// Get a root block given a superblock, or make a new root if there isn't one.
template <class Value>
void get_root(value_sizer_t<Value> *sizer, transaction_t *txn, buf_lock_t& sb_buf, buf_lock_t *buf_out, repli_timestamp_t timestamp) {
    rassert(!buf_out->is_acquired());

    block_id_t node_id = reinterpret_cast<const btree_superblock_t*>(sb_buf->get_data_read())->root_block;

    if (node_id != NULL_BLOCK_ID) {
        buf_lock_t tmp(txn, node_id, rwi_write);
        buf_out->swap(tmp);
    } else {
        buf_out->allocate(txn);
        leaf::init(sizer, reinterpret_cast<leaf_node_t *>(buf_out->buf()->get_data_major_write()), timestamp);
        insert_root(buf_out->buf()->get_block_id(), sb_buf);
    }
}


// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
template <class Value>
void check_and_handle_split(value_sizer_t<Value> *sizer, transaction_t *txn, buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                            const btree_key_t *key, Value *new_value) {
    txn->assert_thread();

    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());

    // If the node isn't full, we don't need to split, so we're done.
    if (node::is_leaf(node)) { // This should only be called when update_needed.
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
    buf_lock_t rbuf;
    rbuf.allocate(txn);
    btree_key_buffer_t median_buffer;
    btree_key_t *median = median_buffer.key();

    node::split(sizer, buf.buf(), reinterpret_cast<node_t *>(rbuf->get_data_major_write()), median);

    // Insert the key that sets the two nodes apart into the parent.
    if (!last_buf.is_acquired()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        last_buf.allocate(txn);
        internal_node::init(sizer->block_size(), reinterpret_cast<internal_node_t *>(last_buf->get_data_major_write()));

        insert_root(last_buf->get_block_id(), sb_buf);
    }

    bool success __attribute__((unused)) = internal_node::insert(sizer->block_size(), last_buf.buf(), median, buf->get_block_id(), rbuf->get_block_id());
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
                                buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                                const btree_key_t *key) {
    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());
    if (last_buf.is_acquired() && node::is_underfull(sizer->block_size(), node)) { // The root node is never underfull.

        const internal_node_t *parent_node = reinterpret_cast<const internal_node_t *>(last_buf->get_data_read());

        // Acquire a sibling to merge or level with.
        block_id_t sib_node_id;
        int nodecmp_node_with_sib = internal_node::sibling(parent_node, key, &sib_node_id);

        // Now decide whether to merge or level.
        buf_lock_t sib_buf(txn, sib_node_id, rwi_write);
        const node_t *sib_node = reinterpret_cast<const node_t *>(sib_buf->get_data_read());

#ifndef NDEBUG
        node::validate(sizer, sib_node);
#endif

        if (node::is_mergable(sizer->block_size(), node, sib_node, parent_node)) { // Merge.

            // This is the key that we remove.
            btree_key_buffer_t key_to_remove_buffer;
            btree_key_t *key_to_remove = key_to_remove_buffer.key();

            if (nodecmp_node_with_sib < 0) { // Nodes must be passed to merge in ascending order.
                node::merge(sizer, node, sib_buf.buf(), key_to_remove, parent_node);
                buf->mark_deleted();
                buf.swap(sib_buf);
            } else {
                node::merge(sizer, sib_node, buf.buf(), key_to_remove, parent_node);
                sib_buf->mark_deleted();
            }

            sib_buf.release();

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(sizer->block_size(), last_buf.buf(), key_to_remove);
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf->mark_deleted();
                insert_root(buf->get_block_id(), sb_buf);
            }
        } else { // Level
            btree_key_buffer_t key_to_replace_buffer, replacement_key_buffer;
            btree_key_t *key_to_replace = key_to_replace_buffer.key();
            btree_key_t *replacement_key = replacement_key_buffer.key();

            bool leveled = node::level(sizer, buf.buf(), sib_buf.buf(), key_to_replace, replacement_key, parent_node);

            if (leveled) {
                internal_node::update_key(last_buf.buf(), key_to_replace, replacement_key);
            }
        }
    }
}

inline void get_btree_superblock(btree_slice_t *slice, access_t access, order_token_t token, got_superblock_t *got_superblock_out) {
    rassert(is_read_mode(access));
    get_btree_superblock(slice, access, 0, repli_timestamp_t::distant_past, token, got_superblock_out);
}

inline void get_btree_superblock(btree_slice_t *slice, access_t access, int expected_change_count, repli_timestamp_t tstamp, order_token_t token, got_superblock_t *got_superblock_out) {
    slice->assert_thread();

    slice->pre_begin_transaction_sink_.check_out(token);
    order_token_t begin_transaction_token = (is_read_mode(access) ? slice->pre_begin_transaction_read_mode_source_ : slice->pre_begin_transaction_write_mode_source_).check_in(token.tag() + "+begin_transaction_token");
    got_superblock_out->txn.reset(new transaction_t(slice->cache(), access, expected_change_count, tstamp));
    got_superblock_out->txn->set_token(slice->post_begin_transaction_checkpoint_.check_through(begin_transaction_token));

    buf_lock_t tmp(got_superblock_out->txn.get(), SUPERBLOCK_ID, access);
    got_superblock_out->sb_buf.swap(tmp);
}

template <class Value>
void find_keyvalue_location_for_write(value_sizer_t<Value> *sizer, got_superblock_t *got_superblock, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t<Value> *keyvalue_location_out) {
    keyvalue_location_out->sb_buf.swap(got_superblock->sb_buf);
    keyvalue_location_out->txn.swap(got_superblock->txn);

    buf_lock_t last_buf;
    buf_lock_t buf;
    get_root(sizer, keyvalue_location_out->txn.get(), keyvalue_location_out->sb_buf, &buf, tstamp);

    // Walk down the tree to the leaf.
    while (node::is_internal(reinterpret_cast<const node_t *>(buf->get_data_read()))) {
        // Check if the node is overfull and proactively split it if it is (since this is an internal node).
        check_and_handle_split(sizer, keyvalue_location_out->txn.get(), buf, last_buf, keyvalue_location_out->sb_buf, key, reinterpret_cast<Value *>(NULL));
        // Check if the node is underfull, and merge/level if it is.
        check_and_handle_underfull(sizer, keyvalue_location_out->txn.get(), buf, last_buf, keyvalue_location_out->sb_buf, key);

        // Release the superblock, if we've gone past the root (and haven't
        // already released it). If we're still at the root or at one of
        // its direct children, we might still want to replace the root, so
        // we can't release the superblock yet.
        if (keyvalue_location_out->sb_buf.is_acquired() && last_buf.is_acquired()) {
            keyvalue_location_out->sb_buf.release();
        }

        // Release the old previous node (unless we're at the root), and set
        // the next previous node (which is the current node).

        // Look up and acquire the next node.
        block_id_t node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf->get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        buf_lock_t tmp(keyvalue_location_out->txn.get(), node_id, rwi_write);
        last_buf.swap(tmp);
        buf.swap(last_buf);
    }

    {
        scoped_malloc<Value> tmp(sizer->max_possible_size());

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        bool key_found = leaf::lookup<Value>(sizer, reinterpret_cast<const leaf_node_t *>(buf->get_data_read()), key, tmp.get());

        if (key_found) {
            keyvalue_location_out->there_originally_was_value = true;
            keyvalue_location_out->value.swap(tmp);
        }
    }

    keyvalue_location_out->last_buf.swap(last_buf);
    keyvalue_location_out->buf.swap(buf);
}

template <class Value>
void find_keyvalue_location_for_read(value_sizer_t<Value> *sizer, got_superblock_t *got_superblock, btree_key_t *key, keyvalue_location_t<Value> *keyvalue_location_out) {
    buf_lock_t buf;
    buf.swap(got_superblock->sb_buf);
    boost::scoped_ptr<transaction_t> txn;
    txn.swap(got_superblock->txn);

    block_id_t node_id = reinterpret_cast<const btree_superblock_t *>(buf->get_data_read())->root_block;
    rassert(node_id != SUPERBLOCK_ID);

    if (node_id == NULL_BLOCK_ID) {
        // There is no root, so the tree is empty.
        keyvalue_location_out->txn.swap(txn);
        return;
    }

    {
        buf_lock_t tmp(txn.get(), node_id, rwi_read);
        buf.swap(tmp);
    }

#ifndef NDEBUG
    node::validate(sizer, reinterpret_cast<const node_t *>(buf->get_data_read()));
#endif  // NDEBUG

    while (node::is_internal(reinterpret_cast<const node_t *>(buf->get_data_read()))) {
        node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf->get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        {
            buf_lock_t tmp(txn.get(), node_id, rwi_read);
            buf.swap(tmp);
        }

#ifndef NDEBUG
        node::validate(sizer, reinterpret_cast<const node_t *>(buf->get_data_read()));
#endif  // NDEBUG
    }

    // Got down to the leaf, now probe it.
    const leaf_node_t *leaf = reinterpret_cast<const leaf_node_t *>(buf->get_data_read());
    int key_index = leaf::impl::find_key(leaf, key);

    if (key_index == leaf::impl::key_not_found) {
        keyvalue_location_out->txn.swap(txn);
        return;
    }

    const Value *value = leaf::get_pair_by_index<Value>(leaf, key_index)->value();

    keyvalue_location_out->txn.swap(txn);
    keyvalue_location_out->buf.swap(buf);
    keyvalue_location_out->there_originally_was_value = true;
    {
        int n = sizer->size(value);
        scoped_malloc<Value> tmp(n);
        memcpy(tmp.get(), value, n);
        keyvalue_location_out->value.swap(tmp);
    }
}

template <class Value>
void apply_keyvalue_change(value_sizer_t<Value> *sizer, keyvalue_location_t<Value> *kv_loc, btree_key_t *key, repli_timestamp_t tstamp) {
    if (kv_loc->value) {
        // We have a value to insert.

        // Split the node if necessary, to make sure that we have room
        // for the value.  Not necessary when deleting, because the
        // node won't grow.

        check_and_handle_split(sizer, kv_loc->txn.get(), kv_loc->buf, kv_loc->last_buf, kv_loc->sb_buf, key, kv_loc->value.get());

        bool success = leaf::insert(sizer, kv_loc->buf.buf(), key, kv_loc->value.get(), tstamp);
        guarantee(success, "could not insert into leaf btree node");
    } else {
        // Delete the value if it's there.
        if (kv_loc->there_originally_was_value) {
            leaf::remove(sizer, kv_loc->buf.buf(), key);
        }
    }

    // Check to see if the leaf is underfull (following a change in
    // size or a deletion, and merge/level if it is.
    check_and_handle_underfull(sizer, kv_loc->txn.get(), kv_loc->buf, kv_loc->last_buf, kv_loc->sb_buf, key);
}

template <class Value>
value_txn_t<Value>::value_txn_t(btree_key_t *key, boost::scoped_ptr<got_superblock_t> &_got_superblock, 
                               boost::scoped_ptr<value_sizer_t<Value> > &_sizer, boost::scoped_ptr<keyvalue_location_t<Value> > &_kv_location, 
                               repli_timestamp_t tstamp)  
    : key(key), tstamp(tstamp)
{ 
    got_superblock.swap(_got_superblock);
    sizer.swap(_sizer);
    kv_location.swap(_kv_location);
    value.swap(kv_location->value);
}

template <class Value>
value_txn_t<Value>::~value_txn_t() {
    kv_location->value.reinterpret_swap(value);
    apply_keyvalue_change(sizer.get(), kv_location.get(), key, tstamp);
}

template <class Value>
value_txn_t<Value> get_value_write(btree_slice_t *slice, btree_key_t *key, const repli_timestamp_t tstamp, const order_token_t token) {
    boost::scoped_ptr<value_sizer_t<Value> > sizer(new value_sizer_t<Value>(slice->cache()->get_block_size()));
    boost::scoped_ptr<got_superblock_t> got_superblock(new got_superblock_t);

    get_btree_superblock(slice, rwi_write, 1, tstamp, token, got_superblock.get());

    boost::scoped_ptr<keyvalue_location_t<Value> > kv_location(new keyvalue_location_t<Value>);
    find_keyvalue_location_for_write(sizer.get(), got_superblock.get(), key, tstamp, kv_location.get());

    value_txn_t<Value> value_txn(key, got_superblock, sizer, kv_location, tstamp);

    return value_txn;
}

template <class Value>
transaction_t *value_txn_t<Value>::get_txn() {
    return kv_location->txn.get();
}

template <class Value>
void get_value_read(btree_slice_t *slice, btree_key_t *key, order_token_t token, keyvalue_location_t<Value> *kv_location_out) {
    value_sizer_t<Value> sizer(slice->cache()->get_block_size());
    got_superblock_t got_superblock;
    get_btree_superblock(slice, rwi_read, token, &got_superblock);

    find_keyvalue_location_for_read(&sizer, &got_superblock, key, kv_location_out);
}
