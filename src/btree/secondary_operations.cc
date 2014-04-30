// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/secondary_operations.hpp"

#include "btree/operations.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "buffer_cache/alt/alt_serialize_onto_blob.hpp"
#include "containers/archive/vector_stream.hpp"

RDB_IMPL_ME_SERIALIZABLE_4(secondary_index_t, 0, superblock, opaque_definition,
                           post_construction_complete, id);

void get_secondary_indexes_internal(buf_lock_t *sindex_block,
                                    std::map<std::string, secondary_index_t> *sindexes_out) {
    buf_read_t read(sindex_block);
    const btree_sindex_block_t *data
        = static_cast<const btree_sindex_block_t *>(read.get_data_read());

    blob_t sindex_blob(sindex_block->cache()->max_block_size(),
                       const_cast<char *>(data->sindex_blob),
                       btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);

    deserialize_from_blob(buf_parent_t(sindex_block), &sindex_blob, sindexes_out);
}

void set_secondary_indexes_internal(buf_lock_t *sindex_block,
                                    const std::map<std::string, secondary_index_t> &sindexes) {
    buf_write_t write(sindex_block);
    btree_sindex_block_t *data
        = static_cast<btree_sindex_block_t *>(write.get_data_write());

    blob_t sindex_blob(sindex_block->cache()->max_block_size(),
                       data->sindex_blob,
                       btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);
    serialize_onto_blob(buf_parent_t(sindex_block), &sindex_blob, sindexes);
}

void initialize_secondary_indexes(buf_lock_t *sindex_block) {
    buf_write_t write(sindex_block);
    btree_sindex_block_t *data
        = static_cast<btree_sindex_block_t *>(write.get_data_write());
    data->magic = btree_sindex_block_t::expected_magic;
    memset(data->sindex_blob, 0, btree_sindex_block_t::SINDEX_BLOB_MAXREFLEN);

    set_secondary_indexes_internal(sindex_block,
                                   std::map<std::string, secondary_index_t>());
}

bool get_secondary_index(buf_lock_t *sindex_block, const std::string &id,
                         secondary_index_t *sindex_out) {
    std::map<std::string, secondary_index_t> sindex_map;

    get_secondary_indexes_internal(sindex_block, &sindex_map);

    auto it = sindex_map.find(id);
    if (it != sindex_map.end()) {
        *sindex_out = it->second;
        return true;
    } else {
        return false;
    }
}

bool get_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         secondary_index_t *sindex_out) {
    std::map<std::string, secondary_index_t> sindex_map;

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
                           std::map<std::string, secondary_index_t> *sindexes_out) {
    get_secondary_indexes_internal(sindex_block, sindexes_out);
}

void set_secondary_index(buf_lock_t *sindex_block, const std::string &id,
                         const secondary_index_t &sindex) {
    std::map<std::string, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    /* We insert even if it already exists overwriting the old value. */
    sindex_map[id] = sindex;
    set_secondary_indexes_internal(sindex_block, sindex_map);
}

void set_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         const secondary_index_t &sindex) {
    std::map<std::string, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    for (auto it = sindex_map.begin(); it != sindex_map.end(); ++it) {
        if (it->second.id == id) {
            guarantee(sindex.id == id, "This shouldn't change the id.");
            it->second = sindex;
        }
    }
    set_secondary_indexes_internal(sindex_block, sindex_map);
}

bool delete_secondary_index(buf_lock_t *sindex_block, const std::string &id) {
    std::map<std::string, secondary_index_t> sindex_map;
    get_secondary_indexes_internal(sindex_block, &sindex_map);

    if (sindex_map.erase(id) == 1) {
        set_secondary_indexes_internal(sindex_block, sindex_map);
        return true;
    } else {
        return false;
    }
}

