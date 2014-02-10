// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/operations.hpp"

#include <stdint.h>

#include "btree/slice.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "containers/archive/vector_stream.hpp"

real_superblock_t::real_superblock_t(buf_lock_t &&sb_buf)
    : sb_buf_(std::move(sb_buf)) {}

void real_superblock_t::release() {
    sb_buf_.reset_buf_lock();
}

block_id_t real_superblock_t::get_root_block_id() {
    buf_read_t read(&sb_buf_);
    return static_cast<const btree_superblock_t *>(read.get_data_read())->root_block;
}

void real_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    buf_write_t write(&sb_buf_);
    btree_superblock_t *sb_data
        = static_cast<btree_superblock_t *>(write.get_data_write());
    sb_data->root_block = new_root_block;
}

block_id_t real_superblock_t::get_stat_block_id() {
    buf_read_t read(&sb_buf_);
    return static_cast<const btree_superblock_t *>(read.get_data_read())->stat_block;
}

void real_superblock_t::set_stat_block_id(const block_id_t new_stat_block) {
    buf_write_t write(&sb_buf_);
    btree_superblock_t *sb_data
        = static_cast<btree_superblock_t *>(write.get_data_write());
    sb_data->stat_block = new_stat_block;
}

block_id_t real_superblock_t::get_sindex_block_id() {
    buf_read_t read(&sb_buf_);
    return static_cast<const btree_superblock_t *>(read.get_data_read())->sindex_block;
}

void real_superblock_t::set_sindex_block_id(const block_id_t new_sindex_block) {
    buf_write_t write(&sb_buf_);
    btree_superblock_t *sb_data
        = static_cast<btree_superblock_t *>(write.get_data_write());
    sb_data->sindex_block = new_sindex_block;
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
    rassert(end - cur >= static_cast<ptrdiff_t>(sizeof(sz_t)), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < static_cast<ptrdiff_t>(sizeof(sz_t))) {
        goto check_failed;
    }
    key_size = *reinterpret_cast<sz_t*>(cur);
    cur += sizeof(sz_t);

    rassert(end - cur >= static_cast<int64_t>(key_size), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < static_cast<int64_t>(key_size)) {
        goto check_failed;
    }
    key_ptr = cur;
    cur += key_size;

    rassert(end - cur >= static_cast<ptrdiff_t>(sizeof(sz_t)), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < static_cast<ptrdiff_t>(sizeof(sz_t))) {
        goto check_failed;
    }
    value_size = *reinterpret_cast<sz_t*>(cur);
    cur += sizeof(sz_t);

    rassert(end - cur >= static_cast<int64_t>(value_size), "Superblock metainfo data is corrupted: walked past the end off the buffer");
    if (end - cur < static_cast<int64_t>(value_size)) {
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

bool get_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             std::vector<char> *value_out) {
    std::vector<char> metainfo;

    {
        buf_read_t read(superblock);
        const btree_superblock_t *data
            = static_cast<const btree_superblock_t *>(read.get_data_read());

        // The const cast is okay because we access the data with access_t::read.
        blob_t blob(superblock->cache()->get_block_size(),
                         const_cast<char *>(data->metainfo_blob),
                         btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock), access_t::read, &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);
        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    if (find_superblock_metainfo_entry(metainfo.data(),
                                       metainfo.data() + metainfo.size(),
                                       key, &verybeg, &size,
                                       &info_begin, &info_end)) {
        value_out->assign(info_begin, info_end);
        return true;
    } else {
        return false;
    }
}

void get_superblock_metainfo(
        buf_lock_t *superblock,
        std::vector<std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out) {
    std::vector<char> metainfo;
    {
        buf_read_t read(superblock);
        const btree_superblock_t *data
            = static_cast<const btree_superblock_t *>(read.get_data_read());

        // The const cast is okay because we access the data with access_t::read
        // and don't write to the blob.
        blob_t blob(superblock->cache()->get_block_size(),
                         const_cast<char *>(data->metainfo_blob),
                         btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock), access_t::read,
                        &group, &acq);

        const int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    for (superblock_metainfo_iterator_t kv_iter(metainfo.data(), metainfo.data() + metainfo.size()); !kv_iter.is_end(); ++kv_iter) {
        superblock_metainfo_iterator_t::key_t key = kv_iter.key();
        superblock_metainfo_iterator_t::value_t value = kv_iter.value();
        kv_pairs_out->push_back(std::make_pair(std::vector<char>(key.second, key.second + key.first), std::vector<char>(value.second, value.second + value.first)));
    }
}

void set_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             const std::vector<char> &value) {
    buf_write_t write(superblock);
    btree_superblock_t *data
        = static_cast<btree_superblock_t *>(write.get_data_write());

    blob_t blob(superblock->cache()->get_block_size(),
                     data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock), access_t::read,
                        &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(buf_parent_t(superblock));

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
        } u;
        rassert(key.size() < UINT32_MAX);
        rassert(value.size() < UINT32_MAX);

        u.y = key.size();
        metainfo.insert(metainfo.end(), u.x, u.x + sizeof(uint32_t));
        metainfo.insert(metainfo.end(), key.begin(), key.end());

        u.y = value.size();
        metainfo.insert(metainfo.end(), u.x, u.x + sizeof(uint32_t));
        metainfo.insert(metainfo.end(), value.begin(), value.end());
    }

    blob.append_region(buf_parent_t(superblock), metainfo.size());

    {
        blob_acq_t acq;
        buffer_group_t write_group;
        blob.expose_all(buf_parent_t(superblock), access_t::write,
                        &write_group, &acq);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(metainfo.size(), metainfo.data());

        buffer_group_copy_data(&write_group, const_view(&group_cpy));
    }
}

void delete_superblock_metainfo(buf_lock_t *superblock,
                                const std::vector<char> &key) {
    buf_write_t write(superblock);
    btree_superblock_t *const data
        = static_cast<btree_superblock_t *>(write.get_data_write());

    blob_t blob(superblock->cache()->get_block_size(),
                     data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock), access_t::read,
                        &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(buf_parent_t(superblock));

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    bool found = find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end);

    rassert(found);

    if (found) {

        std::vector<char>::iterator p = metainfo.begin() + (verybeg - metainfo.data());
        std::vector<char>::iterator q = metainfo.begin() + (info_end - metainfo.data());
        metainfo.erase(p, q);

        blob.append_region(buf_parent_t(superblock), metainfo.size());

        {
            blob_acq_t acq;
            buffer_group_t write_group;
            blob.expose_all(buf_parent_t(superblock), access_t::write,
                            &write_group, &acq);

            buffer_group_t group_cpy;
            group_cpy.add_buffer(metainfo.size(), metainfo.data());

            buffer_group_copy_data(&write_group, const_view(&group_cpy));
        }
    }
}

void clear_superblock_metainfo(buf_lock_t *superblock) {
    buf_write_t write(superblock);
    auto data = static_cast<btree_superblock_t *>(write.get_data_write());
    blob_t blob(superblock->cache()->get_block_size(),
                     data->metainfo_blob,
                     btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
    blob.clear(buf_parent_t(superblock));
}

void insert_root(block_id_t root_id, superblock_t* sb) {
    sb->set_root_block_id(root_id);
}

void ensure_stat_block(superblock_t *sb) {
    const block_id_t node_id = sb->get_stat_block_id();

    if (node_id == NULL_BLOCK_ID) {
        //Create a block
        buf_lock_t temp_lock(buf_parent_t(sb->expose_buf().txn()),
                             alt_create_t::create);
        buf_write_t write(&temp_lock);
        //Make the stat block be the default constructed statblock
        *static_cast<btree_statblock_t *>(write.get_data_write())
            = btree_statblock_t();
        sb->set_stat_block_id(temp_lock.block_id());
    }
}

buf_lock_t get_root(value_sizer_t<void> *sizer, superblock_t *sb) {
    const block_id_t node_id = sb->get_root_block_id();

    if (node_id != NULL_BLOCK_ID) {
        return buf_lock_t(sb->expose_buf(), node_id, access_t::write);
    } else {
        buf_lock_t lock(sb->expose_buf(), alt_create_t::create);
        {
            buf_write_t write(&lock);
            leaf::init(sizer, static_cast<leaf_node_t *>(write.get_data_write()));
        }
        insert_root(lock.block_id(), sb);
        return lock;
    }
}

// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
void check_and_handle_split(value_sizer_t<void> *sizer,
                            buf_lock_t *buf,
                            buf_lock_t *last_buf,
                            superblock_t *sb,
                            const btree_key_t *key, void *new_value) {
    {
        buf_read_t buf_read(buf);
        const node_t *node = static_cast<const node_t *>(buf_read.get_data_read());

        // If the node isn't full, we don't need to split, so we're done.
        if (!node::is_internal(node)) { // This should only be called when update_needed.
            rassert(new_value);
            if (!leaf::is_full(sizer, reinterpret_cast<const leaf_node_t *>(node),
                               key, new_value)) {
                return;
            }
        } else {
            rassert(!new_value);
            if (!internal_node::is_full(reinterpret_cast<const internal_node_t *>(node))) {
                return;
            }
        }
    }

    // Allocate a new node to split into, and some temporary memory to keep
    // track of the median key in the split; then actually split.
    buf_lock_t rbuf(last_buf->empty() ? sb->expose_buf() : buf_parent_t(last_buf),
                    alt_create_t::create);
    store_key_t median_buffer;
    btree_key_t *median = median_buffer.btree_key();

    {
        buf_write_t buf_write(buf);
        buf_write_t rbuf_write(&rbuf);
        node::split(sizer,
                    static_cast<node_t *>(buf_write.get_data_write()),
                    static_cast<node_t *>(rbuf_write.get_data_write()),
                    median);
    }

    // Insert the key that sets the two nodes apart into the parent.
    if (last_buf->empty()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        buf_lock_t temp_buf(sb->expose_buf(), alt_create_t::create);
        last_buf->swap(temp_buf);
        {
            buf_write_t last_write(last_buf);
            internal_node::init(sizer->block_size(),
                                static_cast<internal_node_t *>(last_write.get_data_write()));
        }

        insert_root(last_buf->block_id(), sb);
    }

    {
        buf_write_t last_write(last_buf);
        DEBUG_VAR bool success
            = internal_node::insert(sizer->block_size(),
                                    static_cast<internal_node_t *>(last_write.get_data_write()),
                                    median,
                                    buf->block_id(), rbuf.block_id());
        rassert(success, "could not insert internal btree node");
    }

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
void check_and_handle_underfull(value_sizer_t<void> *sizer,
                                buf_lock_t *buf,
                                buf_lock_t *last_buf,
                                superblock_t *sb,
                                const btree_key_t *key) {
    bool node_is_underfull;
    {
        if (last_buf->empty()) {
            // The root node is never underfull.
            node_is_underfull = false;
        } else {
            buf_read_t buf_read(buf);
            const node_t *const node = static_cast<const node_t *>(buf_read.get_data_read());
            node_is_underfull = node::is_underfull(sizer, node);
        }
    }
    if (node_is_underfull) {
        // Acquire a sibling to merge or level with.
        store_key_t key_in_middle;
        block_id_t sib_node_id;
        int nodecmp_node_with_sib;

        {
            buf_read_t last_buf_read(last_buf);
            const internal_node_t *parent_node
                = static_cast<const internal_node_t *>(last_buf_read.get_data_read());
            nodecmp_node_with_sib = internal_node::sibling(parent_node, key,
                                                           &sib_node_id,
                                                           &key_in_middle);
        }

        // Now decide whether to merge or level.
        buf_lock_t sib_buf(last_buf, sib_node_id, access_t::write);

        bool node_is_mergable;
        {
            buf_read_t sib_buf_read(&sib_buf);
            const node_t *sib_node
                = static_cast<const node_t *>(sib_buf_read.get_data_read());

#ifndef NDEBUG
            node::validate(sizer, sib_node);
#endif

            buf_read_t buf_read(buf);
            const node_t *const node
                = static_cast<const node_t *>(buf_read.get_data_read());
            buf_read_t last_buf_read(last_buf);
            const internal_node_t *parent_node
                = static_cast<const internal_node_t *>(last_buf_read.get_data_read());

            node_is_mergable = node::is_mergable(sizer, node, sib_node, parent_node);
        }

        if (node_is_mergable) {
            // Merge.

            // Nodes must be passed to merge in ascending order.
            if (nodecmp_node_with_sib < 0) {
                {
                    buf_write_t buf_write(buf);
                    buf_write_t sib_buf_write(&sib_buf);
                    buf_read_t last_buf_read(last_buf);
                    const internal_node_t *parent_node
                        = static_cast<const internal_node_t *>(last_buf_read.get_data_read());
                    node::merge(sizer,
                                static_cast<node_t *>(buf_write.get_data_write()),
                                static_cast<node_t *>(sib_buf_write.get_data_write()),
                                parent_node);
                }
                buf->mark_deleted();
                buf->swap(sib_buf);
            } else {
                {
                    buf_write_t sib_buf_write(&sib_buf);
                    buf_write_t buf_write(buf);
                    buf_read_t last_buf_read(last_buf);
                    const internal_node_t *parent_node
                        = static_cast<const internal_node_t *>(last_buf_read.get_data_read());
                    node::merge(sizer,
                                static_cast<node_t *>(sib_buf_write.get_data_write()),
                                static_cast<node_t *>(buf_write.get_data_write()),
                                parent_node);
                }
                sib_buf.mark_deleted();
            }

            sib_buf.reset_buf_lock();

            bool parent_is_singleton;
            {
                buf_read_t last_buf_read(last_buf);
                const internal_node_t *parent_node
                    = static_cast<const internal_node_t *>(last_buf_read.get_data_read());
                parent_is_singleton = internal_node::is_singleton(parent_node);
            }

            if (!parent_is_singleton) {
                buf_write_t last_buf_write(last_buf);
                internal_node::remove(sizer->block_size(),
                                      static_cast<internal_node_t *>(last_buf_write.get_data_write()),
                                      key_in_middle.btree_key());
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf->mark_deleted();
                insert_root(buf->block_id(), sb);
            }
        } else {
            // Level.
            store_key_t replacement_key_buffer;
            btree_key_t *replacement_key = replacement_key_buffer.btree_key();

            bool leveled;
            {
                buf_write_t buf_write(buf);
                buf_write_t sib_buf_write(&sib_buf);
                buf_read_t last_buf_read(last_buf);
                const internal_node_t *parent_node
                    = static_cast<const internal_node_t *>(last_buf_read.get_data_read());
                leveled
                    = node::level(sizer, nodecmp_node_with_sib,
                                  static_cast<node_t *>(buf_write.get_data_write()),
                                  static_cast<node_t *>(sib_buf_write.get_data_write()),
                                  replacement_key, parent_node);
            }

            if (leveled) {
                buf_write_t last_buf_write(last_buf);
                internal_node::update_key(static_cast<internal_node_t *>(last_buf_write.get_data_write()),
                                          key_in_middle.btree_key(),
                                          replacement_key);
            }
        }
    }
}

void get_btree_superblock(txn_t *txn, access_t access,
                          scoped_ptr_t<real_superblock_t> *got_superblock_out) {
    buf_lock_t tmp_buf(buf_parent_t(txn), SUPERBLOCK_ID, access);
    scoped_ptr_t<real_superblock_t> tmp_sb(new real_superblock_t(std::move(tmp_buf)));
    *got_superblock_out = std::move(tmp_sb);
}

void get_btree_superblock_and_txn(cache_conn_t *cache_conn,
                                  access_t superblock_access,
                                  int expected_change_count,
                                  repli_timestamp_t tstamp,
                                  write_durability_t durability,
                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                  scoped_ptr_t<txn_t> *txn_out) {
    // RSI: We should pass a preceding_txn here or something.
    txn_t *txn = new txn_t(cache_conn, durability, tstamp, expected_change_count);

    txn_out->init(txn);

    get_btree_superblock(txn, superblock_access, got_superblock_out);
}

void get_btree_superblock_and_txn_for_backfilling(cache_conn_t *cache_conn,
                                                  alt_cache_account_t *backfill_account,
                                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                                  scoped_ptr_t<txn_t> *txn_out) {
    txn_t *txn = new txn_t(cache_conn, read_access_t::read);
    txn_out->init(txn);
    // KSI: Does using a backfill account needlessly slow other operations down?
    txn->set_account(backfill_account);

    get_btree_superblock(txn, access_t::read, got_superblock_out);
    // KSI: This is bad -- we want to backfill, we don't want to snapshot from the
    // superblock (and therefore secondary indexes)-- we really want to snapshot the
    // subtree underneath the root node.
    (*got_superblock_out)->get()->snapshot_subdag();
}

// KSI: This function is possibly stupid: it's nonsensical to talk about the entire
// cache being snapshotted -- we want some subtree to be snapshotted, at least.
void get_btree_superblock_and_txn_for_reading(cache_conn_t *cache_conn,
                                              cache_snapshotted_t snapshotted,
                                              scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                              scoped_ptr_t<txn_t> *txn_out) {
    txn_t *txn = new txn_t(cache_conn, read_access_t::read);
    txn_out->init(txn);

    get_btree_superblock(txn, access_t::read, got_superblock_out);

    // KSI: As mentioned, snapshotting here is stupid.
    if (snapshotted == CACHE_SNAPSHOTTED_YES) {
        (*got_superblock_out)->get()->snapshot_subdag();
    }
}
