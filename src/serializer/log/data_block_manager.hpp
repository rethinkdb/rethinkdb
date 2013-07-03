// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_
#define SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_

#include <vector>

#include "arch/types.hpp"
#include "containers/bitset.hpp"
#include "containers/priority_queue.hpp"
#include "containers/two_level_array.hpp"
#include "containers/scoped.hpp"
#include "perfmon/types.hpp"
#include "serializer/log/config.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/types.hpp"

class log_serializer_t;

class data_block_manager_t;

class gc_entry_t;

struct gc_entry_less_t {
    bool operator() (const gc_entry_t *x, const gc_entry_t *y);
};

struct buf_write_info_t {
    buf_write_info_t(ser_buffer_t *_buf, uint32_t _ser_block_size,
                     block_id_t _block_id)
        : buf(_buf), ser_block_size(_ser_block_size), block_id(_block_id) { }
    ser_buffer_t *buf;
    uint32_t ser_block_size;
    block_id_t block_id;
};

// Identifies an extent, the time we started writing to the
// extent, whether it's the extent we're currently writing to, and
// describes blocks are garbage.
class gc_entry_t : public intrusive_list_node_t<gc_entry_t> {
public:
    // RSI: Implement the modern behavior of this class.

    /* This constructor is for starting a new extent. */
    explicit gc_entry_t(data_block_manager_t *parent);

    /* This constructor is for reconstructing extents that the LBA tells us contained
       data blocks. */
    gc_entry_t(data_block_manager_t *parent, int64_t offset);

    void destroy();
    ~gc_entry_t();

    // Lists the num_blocks() offsets of the blocks, followed by some upper bound for
    // the last block that is no greater than the extent size.  For example, an
    // extent with three blocks, each of size 4093, might return { 0, 4093, 8192,
    // 12288 }.  The blocks are actually [0, 4093), [4093, 8186), [8192, 12285).
    // Blocks are noncontiguous because each chunk of block writes must start at a
    // discrete device block boundary.
    std::vector<uint32_t> block_boundaries() const;

    // RSI: Document.
    uint32_t back_relative_offset() const;

    // Returns the ostensible size of the block_index'th block.  Note that
    // block_boundaries[i] + block_size(i) <= block_boundaries[i + 1].
    uint32_t block_size(unsigned int block_index) const;

    // Returns block_boundaries()[block_index].
    uint32_t relative_offset(unsigned int block_index) const;

    unsigned int block_index(int64_t offset) const;

    bool new_offset(uint32_t ser_block_size,
                    bool align_to_device_block_size,
                    uint32_t *relative_offset_out,
                    unsigned int *block_index_out);

    unsigned int num_blocks() const {
        guarantee(state != state_reconstructing);
        return block_infos.size();
    }
    // RSI: Does anybody actually use num_garbage_blocks or num_live_blocks?
    unsigned int num_garbage_blocks() const {
        unsigned int count = 0;
        for (auto it = block_infos.begin(); it != block_infos.end(); ++it) {
            if (!it->token_referenced && !it->index_referenced) {
                ++count;
            }
        }
        return count;
    }
    unsigned int num_live_blocks() const { return num_blocks() - num_garbage_blocks(); }

    bool all_garbage() const { return num_live_blocks() == 0; }
    uint64_t garbage_bytes() const;
    bool block_is_garbage(unsigned int block_index) const {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        return !block_infos[block_index].token_referenced && !block_infos[block_index].index_referenced;
    }

    void mark_live_tokenwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        block_infos[block_index].token_referenced = true;
    }

    void mark_garbage_tokenwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        block_infos[block_index].token_referenced = false;
    }

    uint64_t token_bytes() const;

    // The "indexwise" here refers to the LBA index, not the block_index parameter.
    void mark_live_indexwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        block_infos[block_index].index_referenced = true;
    }

    void mark_live_indexwise_with_offset(int64_t block_offset, uint32_t ser_block_size);

    void mark_garbage_indexwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_infos[block_index].index_referenced);
        block_infos[block_index].index_referenced = false;
    }

    bool block_referenced_by_index(unsigned int block_index) const {
        guarantee(block_index < block_infos.size());
        return block_infos[block_index].index_referenced;
    }

    uint64_t index_bytes() const;

    void make_active();

private:
    data_block_manager_t *const parent;

public:
    extent_reference_t extent_ref;

    // When we started writing to the extent (this time).
    const microtime_t timestamp;

    // The PQ entry pointing to us.
    priority_queue_t<gc_entry_t *, gc_entry_less_t>::entry_t *our_pq_entry;

    // True iff the extent has been written to after starting up the serializer.
    bool was_written;

    enum state_t {
        // It has been, or is being, reconstructed from data on disk.
        state_reconstructing,
        // We are currently putting things on this extent. It is equal to
        // last_data_extent.
        state_active,
        // Not active, but not a GC candidate yet. It is in young_extent_queue.
        state_young,
        // Candidate to be GCed. It is in gc_pq.
        state_old,
        // Currently being GCed. It is equal to gc_state.current_entry.
        state_in_gc
    } state;

private:
    struct block_info_t {
        uint32_t relative_offset;
        uint32_t ser_block_size;
        bool token_referenced;
        bool index_referenced;
    };

    // Block information, ordered by relative offset.
    std::vector<block_info_t> block_infos;

    // Used by constructors.
    void add_self_to_parent_entries();

    // Only to be used by the destructor, used to look up the gc entry in the
    // parent's entries array.
    const int64_t extent_offset;

    DISABLE_COPYING(gc_entry_t);
};

class data_block_manager_t {
    friend class gc_entry_t;
    friend class dbm_read_ahead_t;
private:
    struct gc_write_t {
        block_id_t block_id;
        ser_buffer_t *buf;
        int64_t old_offset;
        int64_t new_offset;
        uint32_t ser_block_size;
        gc_write_t(block_id_t i, ser_buffer_t *b, int64_t _old_offset,
                   uint32_t _ser_block_size)
            : block_id(i), buf(b), old_offset(_old_offset), new_offset(0),
              ser_block_size(_ser_block_size) { }
    };

    struct gc_writer_t {
        gc_writer_t(gc_write_t *writes, int num_writes, data_block_manager_t *parent);
        bool done;
    private:
        /* TODO: This should go into log_serializer_t */
        void write_gcs(gc_write_t *writes, int num_writes);
        data_block_manager_t *parent;
    };

public:
    data_block_manager_t(const log_serializer_dynamic_config_t *dynamic_config, extent_manager_t *em,
                         log_serializer_t *serializer, const log_serializer_on_disk_static_config_t *static_config,
                         log_serializer_stats_t *parent);
    ~data_block_manager_t();

    struct metablock_mixin_t {
        int64_t active_extent;
        // RSI: Is this useful anymore?
        uint64_t blocks_in_active_extent;
    } __attribute__((__packed__));

    /* When initializing the database from scratch, call start() with just the
    database FD. When restarting an existing database, call start() with the last
    metablock. */

    static void prepare_initial_metablock(metablock_mixin_t *mb);
    void start_existing(file_t *dbfile, metablock_mixin_t *last_metablock);

    void read(int64_t off_in, uint32_t ser_block_size,
              void *buf_out, file_account_t *io_account);

    /* exposed gc api */
    /* mark a buffer as garbage */
    void mark_garbage(int64_t offset, extent_transaction_t *txn);  // Takes a real int64_t.

    bool is_extent_in_use(unsigned int extent_id) {
        return entries.get(extent_id) != NULL;
    }

    /* r{start,end}_reconstruct functions for safety */
    void start_reconstruct();
    void mark_live(int64_t offset, uint32_t ser_block_size);
    void end_reconstruct();

    /* We must make sure that blocks which have tokens pointing to them don't
    get garbage collected. This interface allows log_serializer to tell us about
    tokens */
    // RSI: Probably these should also take ser_block_size (or merely take block
    // index).
    void mark_live_tokenwise(int64_t offset);
    void mark_garbage_tokenwise(int64_t offset);

    /* garbage collect the extents which meet the gc_criterion */
    void start_gc();

    /* take step in gcing */
    void run_gc();

    void prepare_metablock(metablock_mixin_t *metablock);
    bool do_we_want_to_start_gcing() const;

    struct shutdown_callback_t {
        virtual void on_datablock_manager_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    // The shutdown_callback_t may destroy the data_block_manager.
    bool shutdown(shutdown_callback_t *cb);

    struct gc_disable_callback_t {
        virtual void on_gc_disabled() = 0;
        virtual ~gc_disable_callback_t() {}
    };

    // Always calls the callback, returns true if the callback has
    // already been called.
    bool disable_gc(gc_disable_callback_t *cb);

    // Enables gc, immediately.
    void enable_gc();

    // ratio of garbage to blocks in the system
    double garbage_ratio() const;

    std::vector<counted_t<ls_block_token_pointee_t> >
    many_writes(const std::vector<buf_write_info_t> &writes,
                bool assign_new_block_sequence_id,
                file_account_t *io_account,
                iocallback_t *cb);

    std::vector<std::vector<counted_t<ls_block_token_pointee_t> > >
    gimme_some_new_offsets(const std::vector<buf_write_info_t> &writes);


private:
    void actually_shutdown();

    file_account_t *choose_gc_io_account();

    /* Checks whether the extent is empty and if it is, notifies the extent manager
       and cleans up */
    void check_and_handle_empty_extent(unsigned int extent_id);

    // Tells if we should keep gc'ing, being told the next extent that
    // would be gc'ed.
    bool should_we_keep_gcing(const gc_entry_t&) const;

    // Pops things off young_extent_queue that are no longer young.
    void mark_unyoung_entries();

    // Pops the last gc_entry_t off young_extent_queue and declares it
    // to be not young.
    void remove_last_unyoung_entry();

    bool should_perform_read_ahead(int64_t offset);

    /* internal garbage collection structures */
    struct gc_read_callback_t : public iocallback_t {
        data_block_manager_t *parent;
        void on_io_complete() {
            rassert(parent->gc_state.step() == gc_read);
            parent->run_gc();
        }
    };

    void on_gc_write_done();


    log_serializer_stats_t *const stats;

    // This is permitted to destroy the data_block_manager.
    shutdown_callback_t *shutdown_callback;

    enum state_t {
        state_unstarted,
        state_ready,
        state_shutting_down,
        state_shut_down
    };

    state_t state;

    const log_serializer_dynamic_config_t* const dynamic_config;
    const log_serializer_on_disk_static_config_t* const static_config;

    extent_manager_t *const extent_manager;
    log_serializer_t *const serializer;

    file_t *dbfile;
    scoped_ptr_t<file_account_t> gc_io_account_nice;
    scoped_ptr_t<file_account_t> gc_io_account_high;

    /* Contains a pointer to every gc_entry_t, regardless of what its current state
       is */
    two_level_array_t<gc_entry_t *, MAX_DATA_EXTENTS, (1 << 12)> entries;

    /* Contains every extent in the gc_entry_t::state_reconstructing state */
    intrusive_list_t<gc_entry_t> reconstructed_extents;

    /* Contains the extent in the gc_entry_t::state_active state. */
    gc_entry_t *active_extent;

    /* Contains every extent in the gc_entry_t::state_young state */
    intrusive_list_t< gc_entry_t > young_extent_queue;

    /* Contains every extent in the gc_entry_t::state_old state */
    priority_queue_t<gc_entry_t *, gc_entry_less_t> gc_pq;


    /* Buffer used during GC. */
    std::vector<gc_write_t> gc_writes;

    enum gc_step {
        gc_reconstruct, /* reconstructing on startup */
        gc_ready, /* ready to start */
        gc_read,  /* waiting for reads, acquiring main_mutex */
        gc_write /* waiting for writes */
    };

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
        int get() const { return val; }
    };

    struct gc_state_t {
    private:
        // Which step we're on.  See set_step.
        gc_step step_;

    public:
        // Whether gc is/should be stopped.
        bool should_be_stopped;

        // Outstanding io requests
        int refcount;

        // A buffer for blocks we're transferring.
        scoped_malloc_t<char> gc_blocks;


        // The entry we're currently GCing.
        gc_entry_t *current_entry;

        data_block_manager_t::gc_read_callback_t gc_read_callback;
        data_block_manager_t::gc_disable_callback_t *gc_disable_callback;

        gc_state_t()
            : step_(gc_ready), should_be_stopped(0), refcount(0),
              current_entry(NULL) { }

        ~gc_state_t() { }

        gc_step step() const { return step_; }

        // Sets step_, and calls gc_disable_callback if relevant.
        void set_step(gc_step next_step) {
            if (should_be_stopped && next_step == gc_ready
                && (step_ == gc_read || step_ == gc_write)) {
                rassert(gc_disable_callback);
                gc_disable_callback->on_gc_disabled();
                gc_disable_callback = NULL;
            }

            step_ = next_step;
            rassert(step_ != gc_ready || !gc_blocks.has());
        }
    };

    gc_state_t gc_state;


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
