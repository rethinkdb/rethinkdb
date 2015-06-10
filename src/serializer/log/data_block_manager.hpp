// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_
#define SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_

#include <vector>

#include "arch/types.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/priority_queue.hpp"
#include "containers/scoped.hpp"
#include "containers/two_level_array.hpp"
#include "perfmon/types.hpp"
#include "serializer/log/config.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/types.hpp"

class buf_ptr_t;
class log_serializer_t;
class data_block_manager_t;
class gc_entry_t;

struct gc_entry_less_t {
    bool operator() (const gc_entry_t *x, const gc_entry_t *y);
};

namespace data_block_manager {
struct shutdown_callback_t;  // see log_serializer.hpp.
struct metablock_mixin_t;  // see log_serializer.hpp.
}  // namespace data_block_manager

class data_block_manager_t {
    friend class gc_entry_t;
    friend class dbm_read_ahead_t;

public:
    data_block_manager_t(extent_manager_t *em, log_serializer_t *serializer,
                         const log_serializer_on_disk_static_config_t *static_config,
                         log_serializer_stats_t *parent);
    ~data_block_manager_t();

    /* When initializing the database from scratch, call start() with just the
    database FD. When restarting an existing database, call start() with the last
    metablock. */

    static void prepare_initial_metablock(data_block_manager::metablock_mixin_t *mb);
    void start_existing(file_t *dbfile, data_block_manager::metablock_mixin_t *last_metablock);

    buf_ptr_t read(int64_t off_in, block_size_t block_size,
                 file_account_t *io_account);

    /* exposed gc api */
    /* mark a buffer as garbage */
    void mark_garbage(int64_t offset, extent_transaction_t *txn);  // Takes a real int64_t.

    /* r{start,end}_reconstruct functions for safety */
    void start_reconstruct();
    void mark_live(int64_t offset, block_size_t block_size);
    void end_reconstruct();

    /* We must make sure that blocks which have tokens pointing to them don't
    get garbage collected. This interface allows log_serializer to tell us about
    tokens */
    void mark_live_tokenwise_with_offset(int64_t offset);
    void mark_garbage_tokenwise_with_offset(int64_t offset);

    /* garbage collect the extents which meet the gc_criterion */
    void start_gc();

    void prepare_metablock(data_block_manager::metablock_mixin_t *metablock);
    bool do_we_want_to_start_gcing() const;

    // The shutdown_callback_t may destroy the data_block_manager.
    bool shutdown(data_block_manager::shutdown_callback_t *cb);

    // ratio of garbage to blocks in the system
    double garbage_ratio() const;

    std::vector<counted_t<block_token_t> >
    many_writes(const std::vector<buf_write_info_t> &writes,
                file_account_t *io_account,
                iocallback_t *cb);

    std::vector<std::vector<counted_t<ls_block_token_pointee_t> > >
    gimme_some_new_offsets(const std::vector<buf_write_info_t> &writes);

    bool is_gc_active() const;

private:
    void actually_shutdown();

    struct gc_state_t : public intrusive_list_node_t<gc_state_t>{
    public:
        // The entry we're currently GCing.
        // If the entry becomes empty in the middle of GCing it because
        // of other writes, this pointer should be set to NULL.
        // That will cause the GC to abort.
        gc_entry_t *current_entry;

        gc_state_t()
            : current_entry(NULL) { }
    };

    struct gc_write_t {
        ser_buffer_t *buf;
        int64_t old_offset;
        block_size_t block_size;
        gc_write_t(ser_buffer_t *b, int64_t _old_offset,
                   block_size_t _block_size)
            : buf(b), old_offset(_old_offset),
              block_size(_block_size) { }
    };

    /* Runs in a coroutine and keeps calling `gc_one_extent()` for as long as
    we should keep GCing. */
    void run_gc(gc_state_t *gc_state);

    void gc_one_extent(gc_state_t *gc_state);

    void write_gcs(const std::vector<gc_write_t> &writes, gc_state_t *gc_state);

    // Determine how many GC processes should run concurrently at the moment.
    // Returns a number between 1 and MAX_CONCURRENT_GCS
    size_t compute_gc_concurrency() const;

    // Picks an i/o account for GC to use, based on the current garbage rate
    file_account_t *choose_gc_io_account();

    // Checks whether the extent is empty and if it is, notifies the extent manager
    // and cleans up
    void check_and_handle_empty_extent(uint64_t extent_id);

    // Tells if we should keep gc'ing.
    bool should_we_keep_gcing() const;

    // Checks the size of active_gcs and determines whether at least one
    // GC thread should terminate.
    bool should_terminate_one_gc_thread() const;

    // Pops things off young_extent_queue that are no longer young.
    void mark_unyoung_entries();

    // Pops the last gc_entry_t off young_extent_queue and declares it
    // to be not young.
    void remove_last_unyoung_entry();

    void destroy_entry(gc_entry_t *entry);

    bool should_perform_read_ahead(int64_t offset);

    log_serializer_stats_t *const stats;

    // This is permitted to destroy the data_block_manager.
    data_block_manager::shutdown_callback_t *shutdown_callback;

    enum state_t {
        state_unstarted,
        state_ready,
        state_shutting_down,
        state_shut_down
    };

    state_t state;

    const log_serializer_on_disk_static_config_t* const static_config;

    extent_manager_t *const extent_manager;
    log_serializer_t *const serializer;

    file_t *dbfile;
    scoped_ptr_t<file_account_t> gc_io_account_nice;
    scoped_ptr_t<file_account_t> gc_io_account_high;

    /* Contains a pointer to every gc_entry_t, regardless of what its current state
       is */
    two_level_array_t<gc_entry_t *> entries;

    /* Contains every extent in the gc_entry_t::state_reconstructing state */
    intrusive_list_t<gc_entry_t> reconstructed_extents;

    /* Contains the extent in the gc_entry_t::state_active state. */
    gc_entry_t *active_extent;

    /* Contains every extent in the gc_entry_t::state_young state */
    intrusive_list_t<gc_entry_t> young_extent_queue;

    /* Contains every extent in the gc_entry_t::state_old state */
    priority_queue_t<gc_entry_t *, gc_entry_less_t> gc_pq;

    /* \brief structure to keep track of global stats about the data blocks
     */
    class gc_stat_t {
    private:
        int64_t val;
        perfmon_counter_t *perfmon;
    public:
        explicit gc_stat_t(perfmon_counter_t *_perfmon)
            : val(0), perfmon(_perfmon) { }
        void operator+=(int64_t num);
        void operator-=(int64_t num);
        int64_t get() const { return val; }
    };

    /* The state of all currently active GC coroutines */
    intrusive_list_t<gc_state_t> active_gcs;


    struct gc_stats_t {
        gc_stat_t old_total_block_bytes;
        gc_stat_t old_garbage_block_bytes;
        explicit gc_stats_t(log_serializer_stats_t *);
    };

    gc_stats_t gc_stats;

    DISABLE_COPYING(data_block_manager_t);
};

// Exposed for unit tests.  Returns a super-interval of [block_offset,
// ser_block_size) that is almost appropriate for a read-ahead disk read -- it still
// needs to be stretched to be aligned with disk block boundaries.
void unaligned_read_ahead_interval(const int64_t block_offset,
                                   const uint32_t ser_block_size,
                                   const int64_t extent_size,
                                   const int64_t read_ahead_size,
                                   const std::vector<uint32_t> &boundaries,
                                   int64_t *const offset_out,
                                   int64_t *const end_offset_out);

#endif /* SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_ */
