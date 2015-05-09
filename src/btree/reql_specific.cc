// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "btree/reql_specific.hpp"

#include "btree/secondary_operations.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/binary_blob.hpp"

/* This is the actual structure stored on disk for the superblock of a table's primary or
sindex B-tree. Both of them use the exact same format, but the sindex B-trees don't make
use of the `sindex_block` or `metainfo_blob` fields. */
ATTR_PACKED(struct reql_btree_superblock_t {
    block_magic_t magic;
    block_id_t root_block;
    block_id_t stat_block;
    block_id_t sindex_block;

    static const int METAINFO_BLOB_MAXREFLEN
        = from_ser_block_size_t<DEVICE_BLOCK_SIZE>::cache_size - sizeof(magic)
                                                               - sizeof(root_block)
                                                               - sizeof(stat_block)
                                                               - sizeof(sindex_block);

    char metainfo_blob[METAINFO_BLOB_MAXREFLEN];

    static const block_magic_t expected_magic;
});
static const uint32_t REQL_BTREE_SUPERBLOCK_SIZE = sizeof(reql_btree_superblock_t);

const block_magic_t reql_btree_superblock_t::expected_magic = { { 's', 'u', 'p', 'e' } };

void btree_superblock_ct_asserts() {
    // Just some place to put the CT_ASSERTs
    CT_ASSERT(reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN > 0);
    CT_ASSERT(from_cache_block_size_t<sizeof(reql_btree_superblock_t)>::ser_size
              == DEVICE_BLOCK_SIZE);
}

real_superblock_t::real_superblock_t(buf_lock_t &&sb_buf)
    : sb_buf_(std::move(sb_buf)) {}

real_superblock_t::real_superblock_t(
        buf_lock_t &&sb_buf,
        new_semaphore_acq_t &&write_semaphore_acq)
    : write_semaphore_acq_(std::move(write_semaphore_acq)),
      sb_buf_(std::move(sb_buf)) {}

void real_superblock_t::release() {
    sb_buf_.reset_buf_lock();
}

block_id_t real_superblock_t::get_root_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data
        = static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->root_block;
}

void real_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    buf_write_t write(&sb_buf_);
    reql_btree_superblock_t *sb_data = static_cast<reql_btree_superblock_t *>(
        write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));
    sb_data->root_block = new_root_block;
}

block_id_t real_superblock_t::get_stat_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data =
        static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->stat_block;
}

block_id_t real_superblock_t::get_sindex_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data =
        static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->sindex_block;
}

sindex_superblock_t::sindex_superblock_t(buf_lock_t &&sb_buf)
    : sb_buf_(std::move(sb_buf)) {}

void sindex_superblock_t::release() {
    sb_buf_.reset_buf_lock();
}

block_id_t sindex_superblock_t::get_root_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data
        = static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->root_block;
}

void sindex_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    buf_write_t write(&sb_buf_);
    reql_btree_superblock_t *sb_data = static_cast<reql_btree_superblock_t *>(
        write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));
    sb_data->root_block = new_root_block;
}

block_id_t sindex_superblock_t::get_stat_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data =
        static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->stat_block;
}

block_id_t sindex_superblock_t::get_sindex_block_id() {
    buf_read_t read(&sb_buf_);
    uint32_t sb_size;
    const reql_btree_superblock_t *sb_data =
        static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
    guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);
    return sb_data->sindex_block;
}

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

void btree_slice_t::init_real_superblock(real_superblock_t *superblock,
                                         const std::vector<char> &metainfo_key,
                                         const binary_blob_t &metainfo_value) {
    buf_write_t sb_write(superblock->get());
    auto sb = static_cast<reql_btree_superblock_t *>(
            sb_write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));

    // Properly zero the superblock, zeroing sb->metainfo_blob, in particular.
    memset(sb, 0, REQL_BTREE_SUPERBLOCK_SIZE);

    sb->magic = reql_btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = create_stat_block(buf_parent_t(superblock->get()->txn()));
    sb->sindex_block = NULL_BLOCK_ID;

    set_superblock_metainfo(superblock, metainfo_key, metainfo_value);

    buf_lock_t sindex_block(superblock->get(), alt_create_t::create);
    initialize_secondary_indexes(&sindex_block);
    sb->sindex_block = sindex_block.block_id();
}

void btree_slice_t::init_sindex_superblock(sindex_superblock_t *superblock) {
    buf_write_t sb_write(superblock->get());
    auto sb = static_cast<reql_btree_superblock_t *>(
            sb_write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));

    // Properly zero the superblock, zeroing sb->metainfo_blob, in particular.
    memset(sb, 0, REQL_BTREE_SUPERBLOCK_SIZE);

    sb->magic = reql_btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;
    sb->sindex_block = NULL_BLOCK_ID;
}

btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent,
                             const std::string &identifier,
                             index_type_t index_type)
    : stats(parent,
            (index_type == index_type_t::SECONDARY ? "index-" : "") + identifier),
      cache_(c),
      backfill_account_(cache()->create_cache_account(BACKFILL_CACHE_PRIORITY)) { }

btree_slice_t::~btree_slice_t() { }

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

bool get_superblock_metainfo(real_superblock_t *superblock,
                             const std::vector<char> &key,
                             std::vector<char> *value_out) {
    std::vector<char> metainfo;

    {
        buf_read_t read(superblock->get());
        uint32_t sb_size;
        const reql_btree_superblock_t *data
            = static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
        guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);

        // The const cast is okay because we access the data with access_t::read.
        blob_t blob(superblock->get()->cache()->max_block_size(),
                    const_cast<char *>(data->metainfo_blob),
                    reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock->get()), access_t::read, &group, &acq);

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
        real_superblock_t *superblock,
        std::vector<std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out) {
    std::vector<char> metainfo;
    {
        buf_read_t read(superblock->get());
        uint32_t sb_size;
        const reql_btree_superblock_t *data
            = static_cast<const reql_btree_superblock_t *>(read.get_data_read(&sb_size));
        guarantee(sb_size == REQL_BTREE_SUPERBLOCK_SIZE);

        // The const cast is okay because we access the data with access_t::read
        // and don't write to the blob.
        blob_t blob(superblock->get()->cache()->max_block_size(),
                    const_cast<char *>(data->metainfo_blob),
                    reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock->get()), access_t::read,
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

void set_superblock_metainfo(real_superblock_t *superblock,
                             const std::vector<char> &key,
                             const binary_blob_t &value) {
    std::vector<std::vector<char> > keys = {key};
    std::vector<binary_blob_t> values = {value};
    set_superblock_metainfo(superblock, keys, values);
}

void set_superblock_metainfo(real_superblock_t *superblock,
                             const std::vector<std::vector<char> > &keys,
                             const std::vector<binary_blob_t> &values) {
    buf_write_t write(superblock->get());
    reql_btree_superblock_t *data
        = static_cast<reql_btree_superblock_t *>(
            write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));

    blob_t blob(superblock->get()->cache()->max_block_size(),
                data->metainfo_blob, reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock->get()), access_t::read,
                        &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(buf_parent_t(superblock->get()));

    rassert(keys.size() == values.size());
    auto value_it = values.begin();
    for (auto key_it = keys.begin();
         key_it != keys.end();
         ++key_it, ++value_it) {
        uint32_t *size;
        char *verybeg, *info_begin, *info_end;
        const bool found_entry =
            find_superblock_metainfo_entry(metainfo.data(),
                                           metainfo.data() + metainfo.size(),
                                           *key_it, &verybeg, &size,
                                           &info_begin, &info_end);
        if (found_entry) {
            std::vector<char>::iterator beg = metainfo.begin() + (info_begin - metainfo.data());
            std::vector<char>::iterator end = metainfo.begin() + (info_end - metainfo.data());
            // We must modify *size first because resizing the vector invalidates the pointer.
            rassert(value_it->size() <= UINT32_MAX);
            *size = value_it->size();

            std::vector<char>::iterator p = metainfo.erase(beg, end);

            metainfo.insert(p, static_cast<const uint8_t *>(value_it->data()),
                            static_cast<const uint8_t *>(value_it->data())
                                + value_it->size());
        } else {
            union {
                char x[sizeof(uint32_t)];
                uint32_t y;
            } u;
            rassert(key_it->size() < UINT32_MAX);
            rassert(value_it->size() < UINT32_MAX);

            u.y = key_it->size();
            metainfo.insert(metainfo.end(), u.x, u.x + sizeof(uint32_t));
            metainfo.insert(metainfo.end(), key_it->begin(), key_it->end());

            u.y = value_it->size();
            metainfo.insert(metainfo.end(), u.x, u.x + sizeof(uint32_t));
            metainfo.insert(metainfo.end(), static_cast<const uint8_t *>(value_it->data()),
                            static_cast<const uint8_t *>(value_it->data())
                                + value_it->size());
        }
    }

    blob.append_region(buf_parent_t(superblock->get()), metainfo.size());

    {
        blob_acq_t acq;
        buffer_group_t write_group;
        blob.expose_all(buf_parent_t(superblock->get()), access_t::write,
                        &write_group, &acq);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(metainfo.size(), metainfo.data());

        buffer_group_copy_data(&write_group, const_view(&group_cpy));
    }
}

void delete_superblock_metainfo(real_superblock_t *superblock,
                                const std::vector<char> &key) {
    buf_write_t write(superblock->get());
    reql_btree_superblock_t *const data
        = static_cast<reql_btree_superblock_t *>(
            write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));

    blob_t blob(superblock->get()->cache()->max_block_size(),
                data->metainfo_blob, reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(buf_parent_t(superblock->get()), access_t::read,
                        &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(buf_parent_t(superblock->get()));

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    bool found = find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end);

    rassert(found);

    if (found) {

        std::vector<char>::iterator p = metainfo.begin() + (verybeg - metainfo.data());
        std::vector<char>::iterator q = metainfo.begin() + (info_end - metainfo.data());
        metainfo.erase(p, q);

        blob.append_region(buf_parent_t(superblock->get()), metainfo.size());

        {
            blob_acq_t acq;
            buffer_group_t write_group;
            blob.expose_all(buf_parent_t(superblock->get()), access_t::write,
                            &write_group, &acq);

            buffer_group_t group_cpy;
            group_cpy.add_buffer(metainfo.size(), metainfo.data());

            buffer_group_copy_data(&write_group, const_view(&group_cpy));
        }
    }
}

void clear_superblock_metainfo(real_superblock_t *superblock) {
    buf_write_t write(superblock->get());
    auto data
        = static_cast<reql_btree_superblock_t *>(
            write.get_data_write(REQL_BTREE_SUPERBLOCK_SIZE));
    blob_t blob(superblock->get()->cache()->max_block_size(),
                data->metainfo_blob,
                reql_btree_superblock_t::METAINFO_BLOB_MAXREFLEN);
    blob.clear(buf_parent_t(superblock->get()));
}


void get_btree_superblock(
        txn_t *txn,
        access_t access,
        scoped_ptr_t<real_superblock_t> *got_superblock_out) {
    buf_lock_t tmp_buf(buf_parent_t(txn), SUPERBLOCK_ID, access);
    scoped_ptr_t<real_superblock_t> tmp_sb(new real_superblock_t(std::move(tmp_buf)));
    *got_superblock_out = std::move(tmp_sb);
}

/* Variant for writes that go through a superblock write semaphore */
void get_btree_superblock(
        txn_t *txn,
        UNUSED write_access_t access,
        new_semaphore_acq_t &&write_sem_acq,
        scoped_ptr_t<real_superblock_t> *got_superblock_out) {
    buf_lock_t tmp_buf(buf_parent_t(txn), SUPERBLOCK_ID, access_t::write);
    scoped_ptr_t<real_superblock_t> tmp_sb(
        new real_superblock_t(std::move(tmp_buf), std::move(write_sem_acq)));
    *got_superblock_out = std::move(tmp_sb);
}

void get_btree_superblock_and_txn_for_writing(
        cache_conn_t *cache_conn,
        new_semaphore_t *superblock_write_semaphore,
        UNUSED write_access_t superblock_access,
        int expected_change_count,
        write_durability_t durability,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out) {
    txn_t *txn = new txn_t(cache_conn, durability, expected_change_count);

    txn_out->init(txn);

    /* Acquire a ticket from the superblock_write_semaphore */
    new_semaphore_acq_t sem_acq;
    if(superblock_write_semaphore != nullptr) {
        sem_acq.init(superblock_write_semaphore, 1);
        sem_acq.acquisition_signal()->wait();
    }

    get_btree_superblock(txn, access_t::write, got_superblock_out);
}

void get_btree_superblock_and_txn_for_backfilling(
        cache_conn_t *cache_conn,
        cache_account_t *backfill_account,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out) {
    txn_t *txn = new txn_t(cache_conn, read_access_t::read);
    txn_out->init(txn);
    txn->set_account(backfill_account);

    get_btree_superblock(txn, access_t::read, got_superblock_out);
    (*got_superblock_out)->get()->snapshot_subdag();
}

// KSI: This function is possibly stupid: it's nonsensical to talk about the entire
// cache being snapshotted -- we want some subtree to be snapshotted, at least.
// However, if you quickly release the superblock, you'll release any snapshotting of
// secondary index nodes that you could not possibly access.
void get_btree_superblock_and_txn_for_reading(
        cache_conn_t *cache_conn,
        cache_snapshotted_t snapshotted,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out) {
    txn_t *txn = new txn_t(cache_conn, read_access_t::read);
    txn_out->init(txn);

    get_btree_superblock(txn, access_t::read, got_superblock_out);

    if (snapshotted == CACHE_SNAPSHOTTED_YES) {
        (*got_superblock_out)->get()->snapshot_subdag();
    }
}

