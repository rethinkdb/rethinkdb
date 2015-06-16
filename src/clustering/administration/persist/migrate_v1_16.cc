// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate_v1_16.hpp"

#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"

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

NORETURN void migrate_cluster_metadata (
        UNUSED txn_t *txn,
        UNUSED buf_parent_t buf_parent,
        UNUSED const void *old_superblock,
        UNUSED metadata_file_t::write_txn_t *new_output) {
    /* RSI(raft): Support migration. */
    fail_due_to_user_error("Migration from older versions is not implemented in "
                           "this preview. Please create a new data directory.");
}

NORETURN void migrate_auth_file(
        UNUSED const serializer_filepath_t &path,
        UNUSED metadata_file_t::write_txn_t *destination) {
    /* RSI(raft): Support migration. */
    fail_due_to_user_error("Migration from older versions is not implemented in "
                           "this preview. Please create a new data directory.");
}

}  // namespace migrate_v1_16

