// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/store_metainfo.hpp"

#include "btree/reql_specific.hpp"
#include "containers/archive/buffer_stream.hpp"
#include "containers/archive/vector_stream.hpp"

store_metainfo_manager_t::store_metainfo_manager_t(real_superblock_t *superblock) {
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    // TODO: this is inefficient, cut out the middleman (vector)
    get_superblock_metainfo(superblock, &kv_pairs);
    std::vector<region_t> regions;
    std::vector<binary_blob_t> values;
    for (auto &pair : kv_pairs) {
        region_t region;
        {
            buffer_read_stream_t key(pair.first.data(), pair.first.size());
            archive_result_t res = deserialize_for_metainfo(&key, &region);
            guarantee_deserialization(res, "region");
        }
        regions.push_back(region);
        values.push_back(binary_blob_t(pair.second.begin(), pair.second.end()));
    }
    cache = region_map_t<binary_blob_t>::from_unordered_fragments(
        std::move(regions), std::move(values));;
    rassert(cache.get_domain() == region_t::universe());
}

region_map_t<binary_blob_t> store_metainfo_manager_t::get(
        real_superblock_t *superblock,
        const region_t &region) {
    guarantee(superblock != nullptr);
    return cache.mask(region);
}

void store_metainfo_manager_t::visit(
        real_superblock_t *superblock,
        const region_t &region,
        const std::function<void(const region_t &, const binary_blob_t &)> &cb) {
    guarantee(superblock != nullptr);
    cache.visit(region, cb);
}

void store_metainfo_manager_t::update(
        real_superblock_t *superblock,
        const region_map_t<binary_blob_t> &new_values) {
    cache.update(new_values);

    std::vector<std::vector<char> > keys;
    std::vector<binary_blob_t> values;
    cache.visit(region_t::universe(),
        [&](const region_t &region, const binary_blob_t &value) {
            vector_stream_t key;
            write_message_t wm;
            serialize_for_metainfo(&wm, region);
            key.reserve(wm.size());
            DEBUG_VAR int res = send_write_message(&key, &wm);
            rassert(!res);

            keys.push_back(std::move(key.vector()));
            values.push_back(value);
        });

    set_superblock_metainfo(superblock, keys, values);
}
