// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/secondary_operations.hpp"

#include "btree/operations.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/serialize_onto_blob.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"

RDB_IMPL_SERIALIZABLE_5_SINCE_v1_13(
        secondary_index_t, superblock, opaque_definition,
        post_construction_complete, being_deleted, id);

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

block_magic_t v1_13_sindex_block_magic = { { 's', 'i', 'n', 'd' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v1_14>::value
    = { { 's', 'i', 'n', 'e' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v1_15>::value
    = { { 's', 'i', 'n', 'f' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v1_16>::value
    = { { 's', 'i', 'n', 'g' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v2_0>::value
    = { { 's', 'i', 'n', 'h' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v2_1>::value
    = { { 's', 'i', 'n', 'i' } };
template <>
const block_magic_t
btree_sindex_block_magic_t<cluster_version_t::v2_2_is_latest_disk>::value
    = { { 's', 'i', 'n', 'j' } };


cluster_version_t sindex_block_version(const btree_sindex_block_t *data) {
    if (data->magic == v1_13_sindex_block_magic) {
        fail_due_to_user_error(
            "Found a secondary index from unsupported RethinkDB version 1.13.  "
            "You can migrate this secondary index using RethinkDB 2.0.");
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v1_14>::value) {
        return cluster_version_t::v1_14;
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v1_15>::value) {
        return cluster_version_t::v1_15;
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v1_16>::value) {
        return cluster_version_t::v1_16;
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v2_0>::value) {
        return cluster_version_t::v2_0;
    } else if (data->magic
               == btree_sindex_block_magic_t<cluster_version_t::v2_1>::value) {
        return cluster_version_t::v2_1;
    } else if (data->magic
               == btree_sindex_block_magic_t<
                   cluster_version_t::v2_2_is_latest_disk>::value) {
        return cluster_version_t::v2_2_is_latest_disk;
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

void migrate_secondary_index_block(buf_lock_t *sindex_block,
                                   std::map<sindex_name_t, secondary_index_t> *sindexes_out) {
    guarantee(sindexes_out != nullptr);

    cluster_version_t block_version;
    {
        buf_read_t read(sindex_block);
        const btree_sindex_block_t *data
            = static_cast<const btree_sindex_block_t *>(read.get_data_read());
        block_version = sindex_block_version(data);
    }

    get_secondary_indexes_internal(sindex_block, sindexes_out);
    if (block_version != cluster_version_t::LATEST_DISK) {
        set_secondary_indexes_internal(sindex_block, *sindexes_out);
    }
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

