#define __STDC_LIMIT_MACROS

#include "btree/operations.hpp"

#include <stdint.h>

#include "btree/slice.hpp"
#include "buffer_cache/blob.hpp"

real_superblock_t::real_superblock_t(buf_lock_t *sb_buf) {
    sb_buf_.swap(*sb_buf);
}

void real_superblock_t::release() {
    sb_buf_.release_if_acquired();
}

block_id_t real_superblock_t::get_root_block_id() const {
    rassert(sb_buf_.is_acquired());
    return reinterpret_cast<const btree_superblock_t *>(sb_buf_.get_data_read())->root_block;
}

void real_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    rassert(sb_buf_.is_acquired());
    // We have to const_cast, because set_data unfortunately takes void* pointers, but get_data_read()
    // gives us const data. No way around this (except for making set_data take a const void * again, as it used to be).
    sb_buf_.set_data(const_cast<block_id_t *>(&(static_cast<const btree_superblock_t *>(sb_buf_.get_data_read())->root_block)), &new_root_block, sizeof(new_root_block));
}

block_id_t real_superblock_t::get_stat_block_id() const {
    rassert(sb_buf_.is_acquired());
    return reinterpret_cast<const btree_superblock_t *>(sb_buf_.get_data_read())->stat_block;
}

void real_superblock_t::set_stat_block_id(const block_id_t new_stat_block) {
    rassert(sb_buf_.is_acquired());
    // We have to const_cast, because set_data unfortunately takes void* pointers, but get_data_read()
    // gives us const data. No way around this (except for making set_data take a const void * again, as it used to be).
    sb_buf_.set_data(const_cast<block_id_t *>(&(static_cast<const btree_superblock_t *>(sb_buf_.get_data_read())->stat_block)), &new_stat_block, sizeof(new_stat_block));
}

void real_superblock_t::set_eviction_priority(eviction_priority_t eviction_priority) {
    rassert(sb_buf_.is_acquired());
    sb_buf_.set_eviction_priority(eviction_priority);
}

eviction_priority_t real_superblock_t::get_eviction_priority() {
    rassert(sb_buf_.is_acquired());
    return sb_buf_.get_eviction_priority();
}


bool find_superblock_metainfo_entry(char *beg, char *end, const std::vector<char> &key, char **verybeg_ptr_out,  uint32_t **size_ptr_out, char **beg_ptr_out, char **end_ptr_out) {
    superblock_metainfo_iterator_t::sz_t len = static_cast<superblock_metainfo_iterator_t::sz_t>(key.size());
    for (superblock_metainfo_iterator_t kv_iter(beg, end); !kv_iter.is_end(); ++kv_iter) {
        const superblock_metainfo_iterator_t::key_t& cur_key = kv_iter.key();
        if (len == cur_key.first && std::equal(key.begin(), key.end(), cur_key.second)) {
            *verybeg_ptr_out = kv_iter.record_ptr();
            *size_ptr_out = kv_iter.value_size_ptr();
            *beg_ptr_out = kv_iter.value().second;
            *end_ptr_out = kv_iter.next_record_ptr();
            return true;
        }
    }
    return false;
}

void superblock_metainfo_iterator_t::advance(char * p) {
    char* cur = p;
    if (cur == end) {
        goto check_failed;
    }
    rassert(end - cur >= int(sizeof(sz_t)), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < int(sizeof(sz_t))) {
        goto check_failed;
    }
    key_size = *reinterpret_cast<sz_t*>(cur);
    cur += sizeof(sz_t);

    rassert(end - cur >= int64_t(key_size), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < int64_t(key_size)) {
        goto check_failed;
    }
    key_ptr = cur;
    cur += key_size;

    rassert(end - cur >= int(sizeof(sz_t)), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < int(sizeof(sz_t))) {
        goto check_failed;
    }
    value_size = *reinterpret_cast<sz_t*>(cur);
    cur += sizeof(sz_t);

    rassert(end - cur >= int64_t(value_size), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < int64_t(value_size)) {
        goto check_failed;
    }
    value_ptr = cur;
    cur += value_size;

    pos = p;
    next_pos = cur;

    return;

check_failed:
    pos = next_pos = end;
    key_size = value_size = 0;
    key_ptr = value_ptr = NULL;
}

void superblock_metainfo_iterator_t::operator++() {
    if (!is_end()) {
        advance(next_pos);
    }
}

bool get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, std::vector<char> *value_out) {
    const btree_superblock_t *data = static_cast<const btree_superblock_t *>(superblock->get_data_read());

    // The const cast is okay because we access the data with rwi_read
    // and don't write to the blob.
    blob_t blob(const_cast<char *>(data->metainfo_blob), btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    blob_acq_t acq;
    buffer_group_t group;
    blob.expose_all(txn, rwi_read, &group, &acq);

    int64_t group_size = group.get_size();
    std::vector<char> metainfo(group_size);

    buffer_group_t group_cpy;
    group_cpy.add_buffer(group_size, metainfo.data());

    buffer_group_copy_data(&group_cpy, const_view(&group));

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    if (find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end)) {
        value_out->assign(info_begin, info_end);
        return true;
    } else {
        return false;
    }
}

void get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, std::vector<std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out) {
    const btree_superblock_t *data = static_cast<const btree_superblock_t *>(superblock->get_data_read());

    // The const cast is okay because we access the data with rwi_read
    // and don't write to the blob.
    blob_t blob(const_cast<char *>(data->metainfo_blob), btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
    blob_acq_t acq;
    buffer_group_t group;
    blob.expose_all(txn, rwi_read, &group, &acq);

    int64_t group_size = group.get_size();
    std::vector<char> metainfo(group_size);

    buffer_group_t group_cpy;
    group_cpy.add_buffer(group_size, metainfo.data());

    buffer_group_copy_data(&group_cpy, const_view(&group));

    for (superblock_metainfo_iterator_t kv_iter(metainfo.data(), metainfo.data() + metainfo.size()); !kv_iter.is_end(); ++kv_iter) {
        superblock_metainfo_iterator_t::key_t key = kv_iter.key();
        superblock_metainfo_iterator_t::value_t value = kv_iter.value();
        kv_pairs_out->push_back(std::make_pair(std::vector<char>(key.second, key.second + key.first), std::vector<char>(value.second, value.second + value.first)));
    }
}

void set_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, const std::vector<char> &value) {
    btree_superblock_t *data = static_cast<btree_superblock_t *>(superblock->get_data_major_write());

    blob_t blob(data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(txn, rwi_read, &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(txn);

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    if (find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end)) {
        std::vector<char>::iterator beg = metainfo.begin() + (info_begin - metainfo.data());
        std::vector<char>::iterator end = metainfo.begin() + (info_end - metainfo.data());
        // We must modify *size first because resizing the vector invalidates the pointer.
        rassert(value.size() <= UINT32_MAX);
        *size = value.size();

        std::vector<char>::iterator p = metainfo.erase(beg, end);

        metainfo.insert(p, value.begin(), value.end());
    } else {
        union {
            char x[sizeof(uint32_t)];
            uint32_t y;
        } data;
        rassert(key.size() < UINT32_MAX);
        rassert(value.size() < UINT32_MAX);

        data.y = key.size();
        metainfo.insert(metainfo.end(), data.x, data.x + sizeof(uint32_t));
        metainfo.insert(metainfo.end(), key.begin(), key.end());

        data.y = value.size();
        metainfo.insert(metainfo.end(), data.x, data.x + sizeof(uint32_t));
        metainfo.insert(metainfo.end(), value.begin(), value.end());
    }

    blob.append_region(txn, metainfo.size());

    {
        blob_acq_t acq;
        buffer_group_t write_group;
        blob.expose_all(txn, rwi_write, &write_group, &acq);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(metainfo.size(), metainfo.data());

        buffer_group_copy_data(&write_group, const_view(&group_cpy));
    }
}

void delete_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key) {
    btree_superblock_t *data = static_cast<btree_superblock_t *>(superblock->get_data_major_write());

    blob_t blob(data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(txn, rwi_read, &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(txn);

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    bool found = find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end);

    rassert(found);

    if (found) {

        std::vector<char>::iterator p = metainfo.begin() + (verybeg - metainfo.data());
        std::vector<char>::iterator q = metainfo.begin() + (info_end - metainfo.data());
        metainfo.erase(p, q);

        blob.append_region(txn, metainfo.size());

        {
            blob_acq_t acq;
            buffer_group_t write_group;
            blob.expose_all(txn, rwi_write, &write_group, &acq);

            buffer_group_t group_cpy;
            group_cpy.add_buffer(metainfo.size(), metainfo.data());

            buffer_group_copy_data(&write_group, const_view(&group_cpy));
        }
    }
}

void clear_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock) {
    btree_superblock_t *data = static_cast<btree_superblock_t *>(superblock->get_data_major_write());
    blob_t blob(data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
    blob.clear(txn);
}

void insert_root(block_id_t root_id, superblock_t* sb) {
    sb->set_root_block_id(root_id);
    sb->release(); //XXX it's a little bit weird that we release this from here.
}

void ensure_stat_block(transaction_t *txn, superblock_t *sb, eviction_priority_t stat_block_eviction_priority) {
    rassert(ZERO_EVICTION_PRIORITY < stat_block_eviction_priority);

    block_id_t node_id = sb->get_stat_block_id();

    if (node_id == NULL_BLOCK_ID) {
        //Create a block
        buf_lock_t temp_lock(txn);
        //Make the stat block be the default constructed statblock
        *reinterpret_cast<btree_statblock_t *>(temp_lock.get_data_major_write()) = btree_statblock_t();
        sb->set_stat_block_id(temp_lock.get_block_id());

        temp_lock.set_eviction_priority(stat_block_eviction_priority);
    }
}


// Get a root block given a superblock, or make a new root if there isn't one.
void get_root(value_sizer_t<void> *sizer, transaction_t *txn, superblock_t* sb, buf_lock_t *buf_out, eviction_priority_t root_eviction_priority) {
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
void check_and_handle_split(value_sizer_t<void> *sizer, transaction_t *txn, buf_lock_t *buf, buf_lock_t *last_buf, superblock_t *sb,
                            const btree_key_t *key, void *new_value, eviction_priority_t *root_eviction_priority) {
    txn->assert_thread();

    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());

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
    store_key_t median_buffer;
    btree_key_t *median = median_buffer.btree_key();

    node::split(sizer, buf, reinterpret_cast<node_t *>(rbuf.get_data_major_write()), median);
    rbuf.set_eviction_priority(buf->get_eviction_priority());

    // Insert the key that sets the two nodes apart into the parent.
    if (!last_buf->is_acquired()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        buf_lock_t temp_buf(txn);
        last_buf->swap(temp_buf);
        internal_node::init(sizer->block_size(), reinterpret_cast<internal_node_t *>(last_buf->get_data_major_write()));
        rassert(ZERO_EVICTION_PRIORITY < buf->get_eviction_priority());
        last_buf->set_eviction_priority(decr_priority(buf->get_eviction_priority()));
        *root_eviction_priority = last_buf->get_eviction_priority();

        insert_root(last_buf->get_block_id(), sb);
    }

    bool success UNUSED = internal_node::insert(sizer->block_size(), last_buf, median, buf->get_block_id(), rbuf.get_block_id());
    rassert(success, "could not insert internal btree node");

    // We've split the node; now figure out where the key goes and release the other buf (since we're done with it).
    if (0 >= sized_strcmp(key->contents, key->size, median->contents, median->size)) {
        // The key goes in the old buf (the left one).

        // Do nothing.

    } else {
        // The key goes in the new buf (the right one).
        buf->swap(rbuf);
    }
}

// Merge or level the node if necessary.
void check_and_handle_underfull(value_sizer_t<void> *sizer, transaction_t *txn,
                                buf_lock_t *buf, buf_lock_t *last_buf, superblock_t *sb,
                                const btree_key_t *key) {
    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());
    if (last_buf->is_acquired() && node::is_underfull(sizer, node)) { // The root node is never underfull.

        const internal_node_t *parent_node = reinterpret_cast<const internal_node_t *>(last_buf->get_data_read());

        // Acquire a sibling to merge or level with.
        store_key_t key_in_middle;
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
                buf->mark_deleted();
                buf->swap(sib_buf);
            } else {
                node::merge(sizer, const_cast<node_t *>(sib_node), buf, parent_node);
                sib_buf.mark_deleted();
            }

            sib_buf.release();

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(sizer->block_size(), last_buf, key_in_middle.btree_key());
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf->mark_deleted();
                insert_root(buf->get_block_id(), sb);
            }
        } else { // Level
            store_key_t replacement_key_buffer;
            btree_key_t *replacement_key = replacement_key_buffer.btree_key();

            bool leveled = node::level(sizer, nodecmp_node_with_sib, buf, &sib_buf, replacement_key, parent_node);

            if (leveled) {
                internal_node::update_key(last_buf, key_in_middle.btree_key(), replacement_key);
            }
        }
    }
}

void get_btree_superblock(transaction_t *txn, access_t access, scoped_ptr_t<real_superblock_t> *got_superblock_out) {
    buf_lock_t tmp_buf(txn, SUPERBLOCK_ID, access);
    scoped_ptr_t<real_superblock_t> tmp_sb(new real_superblock_t(&tmp_buf));
    tmp_sb->set_eviction_priority(ZERO_EVICTION_PRIORITY);
    got_superblock_out->init(tmp_sb.release());
}

void get_btree_superblock_and_txn_internal(btree_slice_t *slice, access_t access, int expected_change_count, repli_timestamp_t tstamp,
                                           order_token_t token, cache_snapshotted_t snapshotted,
                                           cache_account_t *cache_account,
                                           scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                           scoped_ptr_t<transaction_t> *txn_out) {
    slice->assert_thread();

    order_token_t pre_begin_txn_token = slice->pre_begin_txn_checkpoint_.check_through(token);

    transaction_t *txn = new transaction_t(slice->cache(), access, expected_change_count, tstamp, pre_begin_txn_token);
    txn_out->init(txn);

    txn->set_account(cache_account);

    if (snapshotted == CACHE_SNAPSHOTTED_YES) {
        txn->snapshot();
    }

    get_btree_superblock(txn, access, got_superblock_out);
}

void get_btree_superblock_and_txn(btree_slice_t *slice, access_t access, int expected_change_count,
                                  repli_timestamp_t tstamp, order_token_t token,
                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                  scoped_ptr_t<transaction_t> *txn_out) {
    get_btree_superblock_and_txn_internal(slice, access, expected_change_count, tstamp, token, CACHE_SNAPSHOTTED_NO, NULL, got_superblock_out, txn_out);
}

void get_btree_superblock_and_txn_for_backfilling(btree_slice_t *slice, order_token_t token,
                                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                                  scoped_ptr_t<transaction_t> *txn_out) {
    get_btree_superblock_and_txn_internal(slice, rwi_read_sync, 0, repli_timestamp_t::distant_past, token, CACHE_SNAPSHOTTED_YES, slice->get_backfill_account(), got_superblock_out, txn_out);
}

void get_btree_superblock_and_txn_for_reading(btree_slice_t *slice, access_t access, order_token_t token,
                                              cache_snapshotted_t snapshotted,
                                              scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                              scoped_ptr_t<transaction_t> *txn_out) {
    rassert(is_read_mode(access));
    get_btree_superblock_and_txn_internal(slice, access, 0, repli_timestamp_t::distant_past, token, snapshotted, NULL, got_superblock_out, txn_out);
}
