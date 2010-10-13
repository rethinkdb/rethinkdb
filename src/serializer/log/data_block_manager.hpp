
#ifndef __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__
#define __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__

#include "arch/arch.hpp"
#include "containers/priority_queue.hpp"
#include "containers/two_level_array.hpp"
#include "extents/extent_manager.hpp"
#include "log_serializer_callbacks.hpp"
#include "serializer/types.hpp"
#include <bitset>
#include <functional>
#include <queue>
#include <utility>
#include <sys/time.h>

// TODO: When we start up, start a new extent rather than continuing on the old extent. The
// remainder of the old extent is taboo because if we shut down badly, we might have written data
// to it but not recorded that we had written data, and if we wrote over that data again then the
// SSD controller would have to move it and there would be a risk of fragmentation.

class data_block_manager_t {
    
public:
    data_block_manager_t(log_serializer_t *ser, extent_manager_t *em, size_t _block_size)
        : shutdown_callback(NULL), state(state_unstarted), serializer(ser),
          extent_manager(em), block_size(_block_size) {}
    ~data_block_manager_t() {
        assert(state == state_unstarted || state == state_shut_down);
    }
    
public:
    struct metablock_mixin_t {
        off64_t last_data_extent;
        unsigned int blocks_in_last_data_extent;
    };

    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. */

public:
    /* data to be serialized to disk with each block */
    struct buf_data_t {
        ser_block_id_t block_id;
    };

public:
    void start(direct_file_t *dbfile);
    void start(direct_file_t *dbfile, metablock_mixin_t *last_metablock);

public:
    bool read(off64_t off_in, void *buf_out, iocallback_t *cb);

    /* The offset that the data block manager chose will be left in off_out as soon as write()
    returns. The callback will be called when the data is actually on disk and it is safe to reuse
    the buffer. */
    bool write(void *buf_in, off64_t *off_out, iocallback_t *cb);

public:
    /* exposed gc api */
    /* mark a buffer as garbage */
    void mark_garbage(off64_t);

    bool is_extent_in_use(unsigned int extent_id) {
        return entries.get(extent_id) != NULL;
    }

    /* r{start,stop}_reconstruct functions for safety */
    void start_reconstruct();
    void mark_live(off64_t);
    void end_reconstruct();

    /* garbage collect the extents which meet the gc_criterion */
    void start_gc();

    /* take step in gcing */
    void run_gc();

public:
    void prepare_metablock(metablock_mixin_t *metablock);

public:
    struct shutdown_callback_t {
        virtual void on_datablock_manager_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

public:
    bool do_we_want_to_start_gcing();

private:
    shutdown_callback_t *shutdown_callback;

private:
    enum state_t {
        state_unstarted,
        state_ready,
        state_shutting_down,
        state_shut_down
    } state;

    log_serializer_t *serializer;
    
    extent_manager_t *extent_manager;
    
    direct_file_t *dbfile;
    size_t block_size;
    off64_t last_data_extent;
    unsigned int blocks_in_last_data_extent;
    
    off64_t gimme_a_new_offset();

private:
    void add_gc_entry();

    // Identifies an extent, the time we started writing to the
    // extent, whether it's the extent we're currently writing to, and
    // describes blocks are garbage.
    struct gc_entry : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, gc_entry> {
    public:
        off64_t offset; /* !< the offset that this extent starts at */
        std::bitset<EXTENT_SIZE / BTREE_BLOCK_SIZE> g_array; /* !< bit array for whether or not each block is garbage */
	typedef uint64_t timestamp_t;
        timestamp_t timestamp; /* !< when we started writing to the extent */
        bool active; /* !< this the extent we're currently writing to? */
	bool young; /* !< this extent is considered young? */
    public:
        gc_entry() {
            timestamp = gc_entry::current_timestamp();
        }
        void print() {
#ifndef NDEBUG
            printf("gc_entry:\n");
            printf("offset: %ld\n", offset);
            for (unsigned int i = 0; i < g_array.size(); i++)
                printf("%.8x:\t%d\n", (unsigned int) (offset + (i * DEVICE_BLOCK_SIZE)), g_array.test(i));
            printf("\n");
            printf("\n");
#endif
        }
	
	// Returns the current timestamp in microseconds.
	static timestamp_t current_timestamp() {
	    struct timeval t;
	    assert(0 == gettimeofday(&t, NULL));
	    return uint64_t(t.tv_sec) * (1000 * 1000) + t.tv_usec;
	}
    };

    struct Less {
        bool operator() (const gc_entry x, const gc_entry y);
    };
    // A priority queue of gc_entrys, by garbage ratio.
    priority_queue_t<gc_entry, Less> gc_pq;


    typedef priority_queue_t<gc_entry, Less>::entry_t gc_pq_entry_t;

    // An array of pointers into the priority queue, indexed by extent
    // id.  (The "extent id" being the extent's offset divided
    // by extent_manager->extent_size.)
    two_level_array_t<gc_pq_entry_t *, MAX_DATA_EXTENTS> entries;
    // A queue of the young entry_t's.  What defines "young"?  Those
    // with recent timestamps?
    std::queue< gc_pq_entry_t *, std::deque< gc_pq_entry_t *, gnew_alloc<gc_pq_entry_t *> > > young_extent_queue;

    void print_entries() {
#ifndef NDEBUG
        for (unsigned int i = 0; i * extent_manager->extent_size < (unsigned int) extent_manager->max_extent(); i++)
            if (entries.get(i) != NULL)
                entries.get(i)->data.print();
#endif
    }

    bool should_we_keep_gcing(const gc_entry);



    void mark_unyoung_entries();
    void remove_last_unyoung_entry();


private:
    /* internal garbage collection structures */
    struct gc_read_callback_t : public iocallback_t {
        data_block_manager_t *parent;
        void on_io_complete(event_t *) {
            parent->run_gc();
        }
    };

    struct gc_write_callback_t : public _write_txn_callback_t{
        public:
            data_block_manager_t *parent;
            void on_serializer_write_txn() {
                parent->run_gc();
            }
    };

    enum gc_step {
        gc_reconstruct, /* reconstructing on startup */
        gc_ready, /* ready to start */
        gc_read,  /* waiting for reads, sending out writes */
        gc_write, /* waiting for writes */
    } gc_step_t;

    struct gc_state_t {
        gc_step step;               /* !< which step we're on */
        int refcount;               /* !< outstanding io reqs */
        char *gc_blocks;            /* !< buffer for blocks we're transferring */
        gc_entry current_entry;     /* !< entry we're currently gcing */
        int blocks_copying;         /* !< how many blocks we're copying */
        data_block_manager_t::gc_read_callback_t gc_read_callback;
        data_block_manager_t::gc_write_callback_t gc_write_callback;
        gc_state_t() : step(gc_ready), refcount(0), blocks_copying(0)
        {
            memalign_alloc_t<DEVICE_BLOCK_SIZE> blocks_buffer_allocator;
            /* TODO this is excessive as soon as we have a bound on how much space we need we should allocate less */
            gc_blocks = (char *) blocks_buffer_allocator.malloc(EXTENT_SIZE);
        }
        ~gc_state_t() {
            free(gc_blocks);
        }
    };

    gc_state_t gc_state;
private:
    /* \brief structure to keep track of global stats about the data blocks
     */
    struct gc_stats_t {
        int total_blocks;
        int garbage_blocks;
        gc_stats_t()
            : total_blocks(0), garbage_blocks(0)
        {}
    };

    gc_stats_t gc_stats;
public:
    /* \brief ratio of garbage to blocks in the system
     */
    float garbage_ratio();
};

#endif /* __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__ */
