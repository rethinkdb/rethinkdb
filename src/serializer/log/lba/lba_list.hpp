// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_LBA_LIST_HPP_
#define SERIALIZER_LOG_LBA_LBA_LIST_HPP_

#include <functional>

#include "concurrency/signal.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/scoped.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/lba/in_memory_index.hpp"
#include "serializer/log/lba/disk_structure.hpp"

class lba_start_fsm_t;
class lba_syncer_t;

class lba_list_t
{
    friend class lba_start_fsm_t;
    friend class lba_syncer_t;

    typedef std::function<void(const signal_t *, file_account_t *)> write_metablock_fun_t;

public:
    typedef lba_metablock_mixin_t metablock_mixin_t;

    explicit lba_list_t(extent_manager_t *em,
                        const write_metablock_fun_t &_write_metablock_fun);
    ~lba_list_t();

    static void prepare_initial_metablock(metablock_mixin_t *mb_out);
    void prepare_metablock(metablock_mixin_t *mb_out);

    struct ready_callback_t {
        virtual void on_lba_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start_existing(file_t *dbfile, metablock_mixin_t *last_metablock,
                        ready_callback_t *cb);

    index_block_info_t get_block_info(block_id_t block);

    // These return individual fields of get_block_info.
    flagged_off64_t get_block_offset(block_id_t block);
    uint32_t get_ser_block_size(block_id_t block);
    block_size_t get_block_size(block_id_t block);
    repli_timestamp_t get_block_recency(block_id_t block);
    segmented_vector_t<repli_timestamp_t> get_block_recencies(block_id_t first,
                                                              block_id_t step);

    /* Returns a block ID such that all blocks that exist are guaranteed to have IDs less than
    that block ID. */
    block_id_t end_block_id();
    block_id_t end_aux_block_id();

#ifndef NDEBUG
    bool is_extent_referenced(int64_t offset);
    bool is_offset_referenced(int64_t offset);
    int extent_refcount(int64_t offset);
#endif

    void set_block_info(block_id_t block, repli_timestamp_t recency,
                        flagged_off64_t offset, uint32_t ser_block_size,
                        file_account_t *io_account,
                        extent_transaction_t *txn);

    struct sync_callback_t {
        virtual void on_lba_sync() = 0;
        virtual ~sync_callback_t() {}
    };
    void sync(file_account_t *io_account, sync_callback_t *cb);

    void consider_gc();

    // The garbage collector must be shut down first through `shutdown_gc()`
    // (must be run in a coroutine). Once that is done, call `shutdown()` to
    // shut down the whole lba_list.
    // The reason for shutting down in two parts like this is because
    // the garbage collector depends on `write_metablock_fun` to be valid,
    // which would create circular dependencies in the shutdown process
    // of log_serializer_t.
    void shutdown_gc();
    void shutdown();

    bool is_any_gc_active() const;

private:
    // Whether we are currently garbage-collecting a shard.
    bool gc_active[LBA_SHARD_FACTOR];
    scoped_ptr_t<auto_drainer_t> gc_drainer;

    write_metablock_fun_t write_metablock_fun;

    extent_manager_t *const extent_manager;

    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_gc_shutting_down,
        state_shut_down
    } state;

    file_t *dbfile;
    scoped_ptr_t<file_account_t> gc_io_account;

    in_memory_index_t in_memory_index;

    // This is a set of inlined LBA entries which are written directly into the
    // metablock. When the array gets full, all inlined LBA entries are moved
    // to the active LBA extent of their respective LBA shards, as computed from
    // their block ids.
    lba_entry_t inline_lba_entries[LBA_NUM_INLINE_ENTRIES];
    int32_t inline_lba_entries_count;
    bool check_inline_lba_full() const;
    void move_inline_entries_to_extents(file_account_t *io_account, extent_transaction_t *txn);
    void add_inline_entry(block_id_t block, repli_timestamp_t recency,
                                flagged_off64_t offset, uint16_t ser_block_size);

    lba_disk_structure_t *disk_structures[LBA_SHARD_FACTOR];

    // Garbage-collect the given shard
    void gc(int lba_shard, auto_drainer_t::lock_t gc_drainer_lock);

    // Returns true if the garbage ratio is bad enough that we want to
    // gc. The integer is which shard to GC.
    bool we_want_to_gc(int i);

    DISABLE_COPYING(lba_list_t);
};

#endif /* SERIALIZER_LOG_LBA_LBA_LIST_HPP_ */
