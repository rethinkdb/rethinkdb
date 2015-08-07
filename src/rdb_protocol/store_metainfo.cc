// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/store_metainfo.hpp"

#include "btree/reql_specific.hpp"
#include "containers/archive/buffer_stream.hpp"
#include "containers/archive/vector_stream.hpp"

store_metainfo_manager_t::store_metainfo_manager_t(real_superblock_t *superblock) {
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    // TODO: this is inefficient, cut out the middleman (vector)
    get_superblock_metainfo(superblock, &kv_pairs, &cache_version);
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

cluster_version_t store_metainfo_manager_t::get_version(
        real_superblock_t *superblock) const {
    guarantee(superblock != nullptr);
    superblock->get()->read_acq_signal()->wait_lazily_unordered();
    return cache_version;
}

region_map_t<binary_blob_t> store_metainfo_manager_t::get(
        real_superblock_t *superblock,
        const region_t &region) const {
    guarantee(cache_version == cluster_version_t::v2_1,
              "Old metainfo needs to be migrated before being used.");
    guarantee(superblock != nullptr);
    superblock->get()->read_acq_signal()->wait_lazily_unordered();
    return cache.mask(region);
}

void store_metainfo_manager_t::visit(
        real_superblock_t *superblock,
        const region_t &region,
        const std::function<void(const region_t &, const binary_blob_t &)> &cb) const {
    guarantee(cache_version == cluster_version_t::v2_1,
              "Old metainfo needs to be migrated before being used.");
    guarantee(superblock != nullptr);
    superblock->get()->read_acq_signal()->wait_lazily_unordered();
    cache.visit(region, cb);
}

void store_metainfo_manager_t::update(
        real_superblock_t *superblock,
        const region_map_t<binary_blob_t> &new_values) {
    guarantee(superblock != nullptr);
    superblock->get()->write_acq_signal()->wait_lazily_unordered();

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

    set_superblock_metainfo(superblock, keys, values, cache_version);
}

void store_metainfo_manager_t::migrate(
        real_superblock_t *superblock,
        cluster_version_t from,
        cluster_version_t to,
        const region_t &region, // This should be for all valid ranges for this hash shard
        const std::function<binary_blob_t(const binary_blob_t &)> &cb) {
    guarantee(superblock != nullptr);
    superblock->get()->write_acq_signal()->wait_lazily_unordered();

    guarantee(cache_version == from);
    region_map_t<binary_blob_t> new_metainfo;
    {
        ASSERT_NO_CORO_WAITING;
        new_metainfo = cache.map(region, cb);
    }
    cache_version = to;
    update(superblock, new_metainfo);
}
