// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_DISK_STRUCTURE_HPP_
#define SERIALIZER_LOG_LBA_DISK_STRUCTURE_HPP_

#include <set>

#include "arch/types.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/lba/disk_extent.hpp"

class lba_load_fsm_t;
class lba_writer_t;

class lba_disk_structure_t :
    public extent_t::read_callback_t
{
    friend class lba_load_fsm_t;
    friend class lba_writer_t;

public:
    // Create a new LBA
    lba_disk_structure_t(extent_manager_t *em, rdb_file_t *file);

    // Load an existing LBA from disk
    struct load_callback_t {
        virtual void on_lba_load() = 0;
        virtual ~load_callback_t() {}
    };
    lba_disk_structure_t(extent_manager_t *em, rdb_file_t *file, lba_shard_metablock_t *metablock);
    void set_load_callback(load_callback_t *lcb);

    // Put entries in an LBA and then call sync() to write to disk
    void add_entry(block_id_t block_id, repli_timestamp_t recency,
                   flagged_off64_t offset, uint32_t ser_block_size,
                   file_account_t *io_account,
                   extent_transaction_t *txn);
    struct sync_callback_t {
        virtual void on_lba_sync() = 0;
        virtual ~sync_callback_t() {}
    };
    void sync(file_account_t *io_account, sync_callback_t *cb);

    // Returns a set of extents that are not currently active.
    // The returned pointers are valid for as long as the LBA disk structure is not
    // `destroy()`ed and `destroy_extents()` is not called on them.
    std::set<lba_disk_extent_t *> get_inactive_extents() const;

    // Destroy the given set of extents. Assumes that the extents pointed to
    // are part of the `extents_in_superblock` list.
    // Once the extents have been destroyed, a new superblock is written to persist
    // the change.
    void destroy_extents(const std::set<lba_disk_extent_t *> &extents,
                         file_account_t *io_account, extent_transaction_t *txn);

    // If you call read(), then the in_memory_index_t will be populated and then the read_callback_t
    // will be called when it is done.
    struct read_callback_t {
        virtual void on_lba_extents_read() = 0;
        virtual ~read_callback_t() {}
    };
    void read(in_memory_index_t *index, read_callback_t *cb);

    void prepare_metablock(lba_shard_metablock_t *mb_out);

    void destroy(extent_transaction_t *txn);   // Delete both in memory and on disk
    void shutdown();   // Delete just in memory

    int num_entries_that_can_fit_in_an_extent() const;

    extent_manager_t *em;
    rdb_file_t *file;

    extent_t *superblock_extent;   // Can be NULL
    int64_t superblock_offset;
    intrusive_list_t<lba_disk_extent_t> extents_in_superblock;
    lba_disk_extent_t *last_extent;

private:
    /* Prepares and writes a new superblock. */
    void write_superblock(file_account_t *io_account, extent_transaction_t *txn);

    /* Used during the startup process */
    void on_extent_read();
    load_callback_t *start_callback;
    int startup_superblock_count;
    lba_superblock_t *startup_superblock_buffer;

    /* Use destroy() or shutdown() instead */
    ~lba_disk_structure_t() {}

    DISABLE_COPYING(lba_disk_structure_t);
};

#endif  // SERIALIZER_LOG_LBA_DISK_STRUCTURE_HPP_
