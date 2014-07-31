// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/secondary_operations.hpp"

#include "btree/operations.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "buffer_cache/alt/serialize_onto_blob.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"

template <>
void
serialize<cluster_version_t::v1_14_is_latest_disk>(write_message_t *wm,
                                                   const secondary_index_t &sindex) {
    const cluster_version_t W = cluster_version_t::v1_14_is_latest_disk;
    serialize<W>(wm, sindex.superblock);
    serialize<W>(wm, sindex.opaque_definition);
    serialize<W>(wm, sindex.post_construction_complete);
    serialize<W>(wm, sindex.being_deleted);
    serialize<W>(wm, sindex.id);
    serialize<W>(wm, sindex.original_reql_version);
    serialize<W>(wm, sindex.latest_compatible_reql_version);
    serialize<W>(wm, sindex.latest_checked_reql_version);
}


template <cluster_version_t W>
typename std::enable_if<W == cluster_version_t::v1_14_is_latest_disk,
                        archive_result_t>::type
deserialize(read_stream_t *s, secondary_index_t *out) {
    archive_result_t res = deserialize<W>(s, &out->superblock);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->opaque_definition);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->post_construction_complete);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->being_deleted);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->id);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->original_reql_version);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->latest_compatible_reql_version);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->latest_checked_reql_version);
    return res;
}

template <cluster_version_t W>
typename std::enable_if<W == cluster_version_t::v1_13 || W == cluster_version_t::v1_13_2,
               archive_result_t>::type
deserialize(read_stream_t *s, secondary_index_t *out) {
    archive_result_t res = deserialize<W>(s, &out->superblock);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->opaque_definition);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->post_construction_complete);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->being_deleted);
    if (bad(res)) { return res; }
    res = deserialize<W>(s, &out->id);
    if (bad(res)) { return res; }
    out->original_reql_version = cluster_version_t::v1_13_2;
    out->latest_compatible_reql_version = cluster_version_t::v1_13_2;
    out->latest_checked_reql_version = cluster_version_t::v1_13_2;
    return archive_result_t::SUCCESS;
}

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(sindex_name_t, name, being_deleted);

struct btree_sindex_block_t {
    static const int SINDEX_BLOB_MAXREFLEN = 4076;

    block_magic_t magic;
    char sindex_blob[SINDEX_BLOB_MAXREFLEN];
} __attribute__((__packed__));

template <cluster_version_t W>
struct btree_sindex_block_magic_t {
    static const block_magic_t value;
};

template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v1_13>::value
    = { { 's', 'i', 'n', 'd' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v1_14_is_latest_disk>::value
    = { { 's', 'i', 'n', 'e' } };

cluster_version_t sindex_block_version(const btree_sindex_block_t *data) {
    if (data->magic
        == btree_sindex_block_magic_t<cluster_version_t::v1_13>::value) {
        return cluster_version_t::v1_13;
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v1_14_is_latest_disk>::value) {
        return cluster_version_t::v1_14_is_latest_disk;
    } else {
        crash("Unexpected magic in btree_sindex_block_t.");
    }
}

void get_secondary_indexes_internal(
        buf_lock_t *sindex_block,
        std::map<sindex_name_t, secondary_index_t> *sindexes_out) {
    buf_read_t read(sindex_block);
    const btree_sindex_block_t *data
        = static_cast<const btree_sindex_block_t *>(read.get_data_read());

    blob_t sindex_blob(sindex_block->cache()->max_block_size(),
                       const_cast<char *>(data->sindex_blob),
                       btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);
    deserialize_for_version_from_blob(sindex_block_version(data),
                                      buf_parent_t(sindex_block), &sindex_blob, sindexes_out);
}

void set_secondary_indexes_internal(
        buf_lock_t *sindex_block,
        const std::map<sindex_name_t, secondary_index_t> &sindexes) {
    buf_write_t write(sindex_block);
    btree_sindex_block_t *data
        = static_cast<btree_sindex_block_t *>(write.get_data_write());

    blob_t sindex_blob(sindex_block->cache()->max_block_size(),
                       data->sindex_blob,
                       btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);
    // There's just one field in btree_sindex_block_t, sindex_blob.  So we set
    // the magic to the latest value and serialize with the latest version.
    data->magic = btree_sindex_block_magic_t<cluster_version_t::LATEST_DISK>::value;
    serialize_onto_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(sindex_block), &sindex_blob, sindexes);
}

void initialize_secondary_indexes(buf_lock_t *sindex_block) {
    buf_write_t write(sindex_block);
    btree_sindex_block_t *data
        = static_cast<btree_sindex_block_t *>(write.get_data_write());
    data->magic = btree_sindex_block_magic_t<cluster_version_t::LATEST_DISK>::value;
    memset(data->sindex_blob, 0, btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);

    set_secondary_indexes_internal(sindex_block,
                                   std::map<sindex_name_t, secondary_index_t>());
}

bool get_secondary_index(buf_lock_t *sindex_block, const sindex_name_t &name,
                         secondary_index_t *sindex_out) {
    std::map<sindex_name_t, secondary_index_t> sindex_map;

    get_secondary_indexes_internal(sindex_block, &sindex_map);

    auto it = sindex_map.find(name);
    if (it != sindex_map.end()) {
        *sindex_out = it->second;
        return true;
    } else {
        return false;
    }
}

bool get_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         secondary_index_t *sindex_out) {
    std::map<sindex_name_t, secondary_index_t> sindex_map;

    get_secondary_indexes_internal(sindex_block, &sindex_map);
    for (auto it = sindex_map.begin(); it != sindex_map.end(); ++it) {
        if (it->second.id == id) {
            *sindex_out = it->second;
            return true;
        }
    }
    return false;
}

void get_secondary_indexes(buf_lock_t *sindex_block,
                           std::map<sindex_name_t, secondary_index_t> *sindexes_out) {
    get_secondary_indexes_internal(sindex_block, sindexes_out);
}

void set_secondary_index(buf_lock_t *sindex_block, const sindex_name_t &name,
                         const secondary_index_t &sindex) {
    std::map<sindex_name_t, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    /* We insert even if it already exists overwriting the old value. */
    sindex_map[name] = sindex;
    set_secondary_indexes_internal(sindex_block, sindex_map);
}

void set_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         const secondary_index_t &sindex) {
    std::map<sindex_name_t, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    for (auto it = sindex_map.begin(); it != sindex_map.end(); ++it) {
        if (it->second.id == id) {
            guarantee(sindex.id == id, "This shouldn't change the id.");
            it->second = sindex;
        }
    }
    set_secondary_indexes_internal(sindex_block, sindex_map);
}

bool delete_secondary_index(buf_lock_t *sindex_block, const sindex_name_t &name) {
    std::map<sindex_name_t, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    if (sindex_map.erase(name) == 1) {
        set_secondary_indexes_internal(sindex_block, sindex_map);
        return true;
    } else {
        return false;
    }
}

