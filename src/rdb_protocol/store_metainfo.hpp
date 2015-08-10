// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STORE_METAINFO_HPP_
#define RDB_PROTOCOL_STORE_METAINFO_HPP_

#include <functional>

#include "containers/binary_blob.hpp"
#include "region/region_map.hpp"

class real_superblock_t;

class store_metainfo_manager_t {
public:
    explicit store_metainfo_manager_t(real_superblock_t *superblock);

    region_map_t<binary_blob_t> get(
        real_superblock_t *superblock,
        const region_t &region) const;

    void visit(
        real_superblock_t *superblock,
        const region_t &region,
        const std::function<void(const region_t &, const binary_blob_t &)> &cb) const;

    void update(
        real_superblock_t *superblock,
        const region_map_t<binary_blob_t> &new_metainfo);

    void migrate(real_superblock_t *superblock,
                 cluster_version_t from,
                 cluster_version_t to,
                 const region_t &region,
                 const std::function<binary_blob_t(const region_t &,
                                                   const binary_blob_t &)> &cb);

    cluster_version_t get_version(real_superblock_t *superblock) const;

private:
    cluster_version_t cache_version;
    region_map_t<binary_blob_t> cache;
};

#endif /* RDB_PROTOCOL_STORE_METAINFO_HPP_ */

