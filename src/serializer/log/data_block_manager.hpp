
#ifndef __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__
#define __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__

#include <functional>
#include <queue>
#include <utility>

#include "arch/arch.hpp"
#include "server/cmd_args.hpp"
#include "containers/priority_queue.hpp"
#include "containers/two_level_array.hpp"
#include "containers/bitset.hpp"
#include "extents/extent_manager.hpp"
#include "serializer/serializer.hpp"
#include "serializer/types.hpp"
#include "perfmon.hpp"
#include "utils2.hpp"

class log_serializer_t;

// Stats

extern perfmon_counter_t
    pm_serializer_data_extents,
    pm_serializer_old_garbage_blocks,
    pm_serializer_old_total_blocks;

//extern perfmon_function_t
//    pm_serializer_garbage_ratio;

class data_block_manager_t
{

    friend class dbm_read_ahead_fsm_t;

private:
    struct gc_write_t {
        ser_block_id_t block_id;
        const void *buf;
        off64_t old_offset;
        gc_write_t(ser_block_id_t i, const void *b, off64_t old_offset) : block_id(i), buf(b), old_offset(old_offset) { }
    };

    struct gc_writer_t {
        gc_writer_t(gc_write_t *writes, int num_writes, data_block_manager_t *parent);
        bool done;
    private:
        void write_gcs(gc_write_t *writes, int num_writes);
        data_block_manager_t *parent;
    };

public:
    data_block_manager_t(const log_serializer_dynamic_config_t *dynamic_config, extent_manager_t *em, log_serializer_t *serializer, const log_serializer_static_config_t *static_config)
        : shutdown_callback(NULL), state(state_unstarted), 
          dynamic_config(dynamic_config), static_config(static_config), extent_manager(em), serializer(serializer),
          next_active_extent(0),
          gc_state(extent_manager->extent_size)//,
//          garbage_ratio_reporter(this)
    {
        rassert(dynamic_config);
        rassert(static_config);
        rassert(extent_manager);
        rassert(serializer);
    }
    ~data_block_manager_t() {
        rassert(state == state_unstarted || state == state_shut_down);
    }

public:
    struct metablock_mixin_t {
        off64_t active_extents[MAX_ACTIVE_DATA_EXTENTS];
        uint64_t blocks_in_active_extent[MAX_ACTIVE_DATA_EXTENTS];
    };

    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. */

public:
    static void prepare_initial_metablock(metablock_mixin_t *mb);
    void start_existing(direct_file_t *dbfile, metablock_mixin_t *last_metablock);

public:
    bool read(off64_t off_in, void *buf_out, iocallback_t *cb);

    /* Returns the offset to which the block will be written */
    off64_t write(const void *buf_in, bool assign_new_block_sequence_id, iocallback_t *cb);

public:
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

public:
    void prepare_metablock(metablock_mixin_t *metablock);
    bool do_we_want_to_start_gcing() const;

public:
    struct shutdown_callback_t {
        virtual void on_datablock_manager_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    // The shutdown_callback_t may destroy the data_block_manager.
    bool shutdown(shutdown_callback_t *cb);

public:

    struct gc_disable_callback_t {
        virtual void on_gc_disabled() = 0;
        virtual ~gc_disable_callback_t() {}
    };

    // Always calls the callback, returns true if the callback has
    // already been called.
    bool disable_gc(gc_disable_callback_t *cb);

    // Enables gc, immediately.
    void enable_gc();

private:
    void actually_shutdown();
    // This is permitted to destroy the data_block_manager.
    shutdown_callback_t *shutdown_callback;

private:
    enum state_t {
        state_unstarted,
        state_ready,
        state_shutting_down,
        state_shut_down
    } state;

    gc_writer_t* gc_writer;

    const log_serializer_dynamic_config_t* const dynamic_config;
    const log_serializer_static_config_t* const static_config;

    extent_manager_t* const extent_manager;
    log_serializer_t *serializer;

    direct_file_t* dbfile;

    off64_t gimme_a_new_offset();

    /* Checks whether the extent is empty and if it is, notifies the extent manager and cleans up */
    void check_and_handle_empty_extent(unsigned int extent_id);
    /* Just pushes the given extent on the potentially_empty_extents queue */
    void check_and_handle_empty_extent_later(unsigned int extent_id);
    std::vector<unsigned int> potentially_empty_extents;
    /* Runs check_and_handle_empty extent() for each extent in potentially_empty_extents */
    void check_and_handle_outstanding_empty_extents();

private:
    
    struct gc_entry;
    
    struct Less {
        bool operator() (const gc_entry *x, const gc_entry *y);
    };

    // Identifies an extent, the time we started writing to the
    // extent, whether it's the extent we're currently writing to, and
    // describes blocks are garbage.
    struct gc_entry :
        public intrusive_list_node_t<gc_entry>
    {
    public:
        data_block_manager_t *parent;
        
        off64_t offset; /* !< the offset that this extent starts at */
        bitset_t g_array; /* !< bit array for whether or not each block is garbage */
        bitset_t t_array; /* !< bit array for whether or not each block is referenced by some token */
        bitset_t i_array; /* !< bit array for whether or not each block is referenced by the current lba (*i*ndex) */
        // Recalculates g_array[block_id] based on t_array[block_id] and i_array[block_id]
        void update_g_array(unsigned int block_id) {
            g_array.set(block_id, t_array[block_id] || i_array[block_id] ? 0 : 1);
        }
        microtime_t timestamp; /* !< when we started writing to the extent */
        priority_queue_t<gc_entry*, Less>::entry_t *our_pq_entry; /* !< The PQ entry pointing to us */
        
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
            state_in_gc,
        } state;
        
    public:
        
        /* This constructor is for starting a new extent. */
        explicit gc_entry(data_block_manager_t *parent)
            : parent(parent),
              offset(parent->extent_manager->gen_extent()),
              g_array(parent->static_config->blocks_per_extent()),
              t_array(parent->static_config->blocks_per_extent()),
              i_array(parent->static_config->blocks_per_extent()),
              timestamp(current_microtime())
        {
            rassert(parent->entries.get(offset / parent->extent_manager->extent_size) == NULL);
            parent->entries.set(offset / parent->extent_manager->extent_size, this);
            g_array.set();
            
            pm_serializer_data_extents++;
        }
        
        /* This constructor is for reconstructing extents that the LBA tells us contained
        data blocks. */
        explicit gc_entry(data_block_manager_t *parent, off64_t offset)
            : parent(parent),
              offset(offset),
              g_array(parent->static_config->blocks_per_extent()),
              t_array(parent->static_config->blocks_per_extent()),
              i_array(parent->static_config->blocks_per_extent()),
              timestamp(current_microtime())
        {
            parent->extent_manager->reserve_extent(offset);
            rassert(parent->entries.get(offset / parent->extent_manager->extent_size) == NULL);
            parent->entries.set(offset / parent->extent_manager->extent_size, this);
            g_array.set();
            
            pm_serializer_data_extents++;
        }
        
        void destroy() {
            parent->extent_manager->release_extent(offset);
            delete this;
        }
        
        ~gc_entry() {
            rassert(parent->entries.get(offset / parent->extent_manager->extent_size) == this);
            parent->entries.set(offset / parent->extent_manager->extent_size, NULL);
            
            pm_serializer_data_extents--;
        }
        
        void print() {
#ifndef NDEBUG
            debugf("gc_entry:\n");
            debugf("offset: %ld\n", offset);
            for (unsigned int i = 0; i < g_array.size(); i++)
                debugf("%.8x:\t%d\n", (unsigned int) (offset + (i * DEVICE_BLOCK_SIZE)), g_array.test(i));
            debugf("\n");
            debugf("\n");
#endif
        }
    };
    
    /* Contains a pointer to every gc_entry, regardless of what its current state is */
    two_level_array_t<gc_entry*, MAX_DATA_EXTENTS> entries;
    
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
    priority_queue_t<gc_entry*, Less> gc_pq;
    
    // Tells if we should keep gc'ing, being told the next extent that
    // would be gc'ed.
    bool should_we_keep_gcing(const gc_entry&) const;

    // Pops things off young_extent_queue that are no longer young.
    void mark_unyoung_entries();
    
    // Pops the last gc_entry off young_extent_queue and declares it
    // to be not young.
    void remove_last_unyoung_entry();

private:
    /* internal garbage collection structures */
    struct gc_read_callback_t : public iocallback_t {
        data_block_manager_t *parent;
        void on_io_complete() {
            parent->run_gc();
        }
    };

    void on_gc_write_done();

    enum gc_step {
        gc_reconstruct, /* reconstructing on startup */
        gc_ready, /* ready to start */
        gc_read,  /* waiting for reads, sending out writes */
        gc_write, /* waiting for writes */
    };

    /* Buffer used during GC. */
    std::vector<gc_write_t> gc_writes;

    struct gc_state_t {
    private:
        gc_step step_;               /* !< which step we're on.  See set_step.  */
    public:
        bool should_be_stopped;      /* !< whether gc is/should be
                                       stopped, and how many people
                                       think so */
        int refcount;               /* !< outstanding io reqs */
        char *gc_blocks;            /* !< buffer for blocks we're transferring */
        gc_entry *current_entry;    /* !< entry we're currently GCing */
        data_block_manager_t::gc_read_callback_t gc_read_callback;
        data_block_manager_t::gc_disable_callback_t *gc_disable_callback;

        explicit gc_state_t(size_t extent_size) : step_(gc_ready), should_be_stopped(0), refcount(0), current_entry(NULL)
        {
            /* TODO this is excessive as soon as we have a bound on how much space we need we should allocate less */
            gc_blocks = (char *)malloc_aligned(extent_size, DEVICE_BLOCK_SIZE);
        }
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
    } gc_state;

private:
    /* \brief structure to keep track of global stats about the data blocks
     */
    class gc_stat_t {
        private: 
            int val;
            perfmon_counter_t &perfmon;
        public:
            explicit gc_stat_t(perfmon_counter_t &perfmon)
                : val(0), perfmon(perfmon)
            {}
            void operator++(int) { val++; perfmon++;}
            void operator+=(int64_t num) { val += num; perfmon += num; }
            void operator--(int) { val--; perfmon--;}
            void operator-=(int64_t num) { val -= num; perfmon -= num;}
            int get() const {return val;}
    };

    struct gc_stats_t {
        gc_stat_t old_total_blocks;
        gc_stat_t old_garbage_blocks;
        gc_stats_t()
            : old_total_blocks(pm_serializer_old_total_blocks), old_garbage_blocks(pm_serializer_old_garbage_blocks)
        {}
    } gc_stats;

public:
    /* \brief ratio of garbage to blocks in the system
     */
    float garbage_ratio() const;

    /* \brief perfmon to output the garbage ratio
     */
    
/*    class garbage_ratio_reporter_t :
        public perfmon_function_t::internal_function_t
    {
    private: 
        data_block_manager_t *data_block_manager;
    public:
        explicit garbage_ratio_reporter_t(data_block_manager_t *data_block_manager)
            : perfmon_function_t::internal_function_t(&pm_serializer_garbage_ratio),
              data_block_manager(data_block_manager) {}
        ~garbage_ratio_reporter_t() {}
        std::string compute_stat() {
            return format(data_block_manager->garbage_ratio());
        }
    } garbage_ratio_reporter;
*/
    int64_t garbage_ratio_total_blocks() const { return gc_stats.old_garbage_blocks.get(); }
    int64_t garbage_ratio_garbage_blocks() const { return gc_stats.old_garbage_blocks.get(); }


private:
    DISABLE_COPYING(data_block_manager_t);
};

#endif /* __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__ */
