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

class gc_entry;

struct gc_entry_less {
    bool operator() (const gc_entry *x, const gc_entry *y);
};


// Identifies an extent, the time we started writing to the
// extent, whether it's the extent we're currently writing to, and
// describes blocks are garbage.
class gc_entry :
    public intrusive_list_node_t<gc_entry>
{
public:
    data_block_manager_t *parent;

    off64_t offset; /* !< the offset that this extent starts at */
    bitset_t g_array; /* !< bit array for whether or not each block is garbage */
    bitset_t t_array; /* !< bit array for whether or not each block is referenced by some token */
    bitset_t i_array; /* !< bit array for whether or not each block is referenced by the current lba (*i*ndex) */
    // g_array is redundant. g_array[i] = !(t_array[i] || i_array[i])
    void update_g_array(unsigned int block_id) {
        g_array.set(block_id, !(t_array[block_id] || i_array[block_id]));
    }
    microtime_t timestamp; /* !< when we started writing to the extent */
    priority_queue_t<gc_entry*, gc_entry_less>::entry_t *our_pq_entry; /* !< The PQ entry pointing to us */
    bool was_written; /* true iff the extent has been written to after starting up the serializer */

    enum state_t {
        // It has been, or is being, reconstructed from data on disk.
        state_reconstructing,
        // We are currently putting things on this extent. It is equal to last_data_extent.
        state_active,
        // Not active, but not a GC candidate yet. It is in young_extent_queue.
        state_young,
        // Candidate to be GCed. It is in gc_pq.
        state_old,
        // Currently being GCed. It is equal to gc_state.current_entry.
        state_in_gc
    } state;

public:
    /* This constructor is for starting a new extent. */
    explicit gc_entry(data_block_manager_t *parent);

    /* This constructor is for reconstructing extents that the LBA tells us contained
       data blocks. */
    gc_entry(data_block_manager_t *parent, off64_t offset);

    void destroy();
    ~gc_entry();

#ifndef NDEBUG
    void print();
#endif

private:
    DISABLE_COPYING(gc_entry);
};

class data_block_manager_t {
    friend class gc_entry;
    friend class dbm_read_ahead_fsm_t;

private:
    struct gc_write_t {
        block_id_t block_id;
        const void *buf;
        off64_t old_offset;
        off64_t new_offset;
        gc_write_t(block_id_t i, const void *b, off64_t _old_offset)
            : block_id(i), buf(b), old_offset(_old_offset), new_offset(0) { }
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
        off64_t active_extents[MAX_ACTIVE_DATA_EXTENTS];
        uint64_t blocks_in_active_extent[MAX_ACTIVE_DATA_EXTENTS];
    };

    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. */

    static void prepare_initial_metablock(metablock_mixin_t *mb);
    void start_existing(direct_file_t *dbfile, metablock_mixin_t *last_metablock);

    void read(off64_t off_in, void *buf_out, file_account_t *io_account, iocallback_t *cb);

    /* Returns the offset to which the block will be written */
    off64_t write(const void *buf_in, block_id_t block_id, bool assign_new_block_sequence_id,
                  file_account_t *io_account, iocallback_t *cb,
                  bool token_referenced, bool index_referenced);

    /* exposed gc api */
    /* mark a buffer as garbage */
    void mark_garbage(off64_t);  // Takes a real off64_t.

    bool is_extent_in_use(unsigned int extent_id) {
        return entries.get(extent_id) != NULL;
    }

    /* r{start,stop}_reconstruct functions for safety */
    void start_reconstruct();
    void mark_live(off64_t);  // Takes a real off64_t.
    void end_reconstruct();

    /* We must make sure that blocks which have tokens pointing to them don't
    get garbage collected. This interface allows log_serializer to tell us about
    tokens */
    void mark_token_live(off64_t);
    void mark_token_garbage(off64_t);

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

    int64_t garbage_ratio_total_blocks() const { return gc_stats.old_garbage_blocks.get(); }
    int64_t garbage_ratio_garbage_blocks() const { return gc_stats.old_garbage_blocks.get(); }

private:
    void actually_shutdown();

    file_account_t *choose_gc_io_account();

    off64_t gimme_a_new_offset(bool token_referenced, bool index_referenced);

    /* Checks whether the extent is empty and if it is, notifies the extent manager and cleans up */
    void check_and_handle_empty_extent(unsigned int extent_id);
    /* Just pushes the given extent on the potentially_empty_extents queue */
    void check_and_handle_empty_extent_later(unsigned int extent_id);
    /* Runs check_and_handle_empty extent() for each extent in potentially_empty_extents */
    void check_and_handle_outstanding_empty_extents();

    // Tells if we should keep gc'ing, being told the next extent that
    // would be gc'ed.
    bool should_we_keep_gcing(const gc_entry&) const;

    // Pops things off young_extent_queue that are no longer young.
    void mark_unyoung_entries();

    // Pops the last gc_entry off young_extent_queue and declares it
    // to be not young.
    void remove_last_unyoung_entry();

    bool should_perform_read_ahead(off64_t offset);

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

    direct_file_t *dbfile;
    scoped_ptr_t<file_account_t> gc_io_account_nice;
    scoped_ptr_t<file_account_t> gc_io_account_high;

    std::vector<unsigned int> potentially_empty_extents;

    /* Contains a pointer to every gc_entry, regardless of what its current state is */
    two_level_array_t<gc_entry *, MAX_DATA_EXTENTS> entries;

    /* Contains every extent in the gc_entry::state_reconstructing state */
    intrusive_list_t< gc_entry > reconstructed_extents;

    /* Contains the extents in the gc_entry::state_active state. The number of active extents
    is determined by dynamic_config->num_active_data_extents. */
    unsigned int next_active_extent;   // Cycles through the active extents
    gc_entry *active_extents[MAX_ACTIVE_DATA_EXTENTS];
    unsigned blocks_in_active_extent[MAX_ACTIVE_DATA_EXTENTS];

    /* Contains every extent in the gc_entry::state_young state */
    intrusive_list_t< gc_entry > young_extent_queue;

    /* Contains every extent in the gc_entry::state_old state */
    priority_queue_t<gc_entry*, gc_entry_less> gc_pq;


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
        int val;
        perfmon_counter_t *perfmon;
    public:
        explicit gc_stat_t(perfmon_counter_t *_perfmon)
            : val(0), perfmon(_perfmon) { }
        void operator++();
        void operator+=(int64_t num);
        void operator--();
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
        char *const gc_blocks;

        // The entry we're currently GCing.
        gc_entry *current_entry;

        data_block_manager_t::gc_read_callback_t gc_read_callback;
        data_block_manager_t::gc_disable_callback_t *gc_disable_callback;

        explicit gc_state_t(size_t extent_size)
            : step_(gc_ready), should_be_stopped(0), refcount(0),
              gc_blocks(static_cast<char *>(malloc_aligned(extent_size, DEVICE_BLOCK_SIZE))),
              current_entry(NULL) { }

        ~gc_state_t() {
            free(gc_blocks);
        }

        inline gc_step step() const { return step_; }

        // Sets step_, and calls gc_disable_callback if relevant.
        void set_step(gc_step next_step) {
            if (should_be_stopped && next_step == gc_ready && (step_ == gc_read || step_ == gc_write)) {
                rassert(gc_disable_callback);
                gc_disable_callback->on_gc_disabled();
                gc_disable_callback = NULL;
            }

            step_ = next_step;
        }
    };

    gc_state_t gc_state;


    struct gc_stats_t {
        gc_stat_t old_total_blocks;
        gc_stat_t old_garbage_blocks;
        explicit gc_stats_t(log_serializer_stats_t *);
    };

    gc_stats_t gc_stats;

    DISABLE_COPYING(data_block_manager_t);
};

#endif /* SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP_ */
