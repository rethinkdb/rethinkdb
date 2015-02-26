// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist_v1_16.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "concurrency/throttled_committer.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "serializer/config.hpp"

namespace migrate_v1_16 {

struct auth_metadata_superblock_t {
    block_magic_t magic;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

};

struct cluster_metadata_superblock_t {
    block_magic_t magic;

    server_id_t server_id;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

    static const int BRANCH_HISTORY_BLOB_MAXREFLEN = 500;
    char rdb_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
};

void migrate_cluster_metadata(
        txn_t *txn,
        buf_parent_t buf_parent,
        const void *old_superblock,
        metadata_file_t::write_txn_t *new_output) {
    /* RSI(raft): Support migration. */
    crash("Migration from v1.16 is not implemented.");
}

}  // namespace migrate_v1_16

