// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_LBA_LIST_HPP_
#define SERIALIZER_LOG_LBA_LBA_LIST_HPP_

#include "serializer/serializer.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/lba/in_memory_index.hpp"
#include "serializer/log/lba/disk_structure.hpp"

class lba_start_fsm_t;
class lba_syncer_t;
class gc_fsm_t;

class lba_list_t
{
    friend class lba_start_fsm_t;
    friend class lba_syncer_t;
    friend class gc_fsm_t;

public:
    typedef lba_metablock_mixin_t metablock_mixin_t;

public:
    explicit lba_list_t(extent_manager_t *em);
    ~lba_list_t();

public:
    static void prepare_initial_metablock(metablock_mixin_t *mb);

    struct ready_callback_t {
        virtual void on_lba_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start_existing(direct_file_t *dbfile, metablock_mixin_t *last_metablock, ready_callback_t *cb);

public:
    flagged_off64_t get_block_offset(block_id_t block);
    repli_timestamp_t get_block_recency(block_id_t block);

    /* Returns a block ID such that all blocks that exist are guaranteed to have IDs less than
    that block ID. */
    block_id_t end_block_id();

#ifndef NDEBUG
    bool is_extent_referenced(off64_t offset);
    bool is_offset_referenced(off64_t offset);
    int extent_refcount(off64_t offset);
#endif

public:
    void set_block_info(block_id_t block, repli_timestamp_t recency,
                        flagged_off64_t offset, file_account_t *io_account,
                        extent_transaction_t *txn);

    struct sync_callback_t {
        virtual void on_lba_sync() = 0;
        virtual ~sync_callback_t() {}
    };
    bool sync(file_account_t *io_account, sync_callback_t *cb);

    void prepare_metablock(metablock_mixin_t *mb_out);

    void consider_gc(file_account_t *io_account, extent_transaction_t *txn);

public:
    struct shutdown_callback_t {
        virtual void on_lba_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    bool shutdown(shutdown_callback_t *cb);

private:
    shutdown_callback_t *shutdown_callback;
    int gc_count;   // Number of active GC fsms
    bool shutdown_now();

private:
    extent_manager_t *const extent_manager;

    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shutting_down,
        state_shut_down
    } state;

    direct_file_t *dbfile;

    in_memory_index_t in_memory_index;

    lba_disk_structure_t *disk_structures[LBA_SHARD_FACTOR];

    // Garbage-collect the given shard
    void gc(int i, file_account_t *io_account, extent_transaction_t *txn);

    // Returns true if the garbage ratio is bad enough that we want to
    // gc. The integer is which shard to GC.
    bool we_want_to_gc(int i);

    DISABLE_COPYING(lba_list_t);
};

#endif /* SERIALIZER_LOG_LBA_LBA_LIST_HPP_ */
