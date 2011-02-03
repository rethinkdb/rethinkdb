
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include "concurrency/rwi_lock.hpp"
#include "flush_time_randomizer.hpp"
#include "utils.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "buffer_cache/mirrored/callbacks.hpp"
#include "buffer_cache/buf_patch.hpp"

struct mc_cache_t;
struct mc_buf_t;
struct mc_inner_buf_t;
struct mc_transaction_t;

struct writeback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_transaction_t transaction_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;
    
public:
    writeback_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold,
        unsigned int max_dirty_blocks,
        unsigned int flush_waiting_threshold,
        unsigned int max_concurrent_flushes
        );
    virtual ~writeback_t();
    
    struct sync_callback_t : public intrusive_list_node_t<sync_callback_t> {
        virtual ~sync_callback_t() {}
        virtual void on_sync() = 0;
    };
    
    /* Forces a writeback to happen soon. If there is nothing to write, return 'true'; otherwise,
    returns 'false' and calls 'callback' as soon as the next writeback cycle is over. */
    bool sync(sync_callback_t *callback);
    
    /* Same as sync(), but doesn't hurry up the writeback in any way. */
    bool sync_patiently(sync_callback_t *callback);

    bool begin_transaction(transaction_t *txn, transaction_begin_callback_t *cb);
    void on_transaction_commit(transaction_t *txn);

    struct begin_transaction_fsm_t :
        public thread_message_t,
        public lock_available_callback_t,
        public intrusive_list_node_t<begin_transaction_fsm_t>
    {
        writeback_t *writeback;
        mc_transaction_t *transaction;
        begin_transaction_fsm_t(writeback_t *wb, mc_transaction_t *txn, mc_transaction_begin_callback_t *cb);
        void green_light();
        void on_thread_switch();
        void on_lock_available();
    };
    intrusive_list_t<begin_transaction_fsm_t> throttled_transactions_list;

    unsigned int num_dirty_blocks();

    bool too_many_dirty_blocks();
    void possibly_unthrottle_transactions();

    class local_buf_t : public intrusive_list_node_t<local_buf_t> {
        friend class writeback_t;
        friend class concurrent_flush_t;
        
    public:
        explicit local_buf_t(inner_buf_t *gbuf)
            : needs_flush(false), last_patch_materialized(0), gbuf(gbuf), dirty(false), recency_dirty(false) {}
        
        void set_dirty(bool _dirty = true);
        void set_recency_dirty(bool _recency_dirty = true);
        
        bool safe_to_unload() const { return !dirty && !recency_dirty; }

        /* true if we have to flush the block instead of just flushing patches. */
        /* Specifically, this is the case if we modified the block while bypassing the patching system */
        bool needs_flush;

        /* All patches <= last_patch_materialized are in the on-disk log storage */
        patch_counter_t last_patch_materialized;

    private:
        inner_buf_t *gbuf;
    public: //TODO make this private again @jdoliner
        bool dirty;
        bool recency_dirty;
    };
    
    /* User-controlled settings. */
    
    bool wait_for_flush;
    unsigned int flush_waiting_threshold;
    unsigned int max_concurrent_flushes;
    
    unsigned int max_dirty_blocks;   // Number of blocks, not percentage
    
private:
    flush_time_randomizer_t flush_time_randomizer;
    unsigned int flush_threshold;   // Number of blocks, not percentage
    
private:
    friend class buf_writer_t;
    friend class concurrent_flush_t;

    // The writeback system has a mechanism to keep data safe if the server crashes. If modified
    // data sits in memory for longer than flush_timer_ms milliseconds, a writeback will be
    // automatically started to store it on disk. flush_timer is the timer to keep track of how
    // much longer the data can sit in memory.
    timer_token_t *flush_timer;
    static void flush_timer_callback(void *ctx);
    
    /* The number of writes that we dispatched to the disk that have not come back yet. */
    unsigned long long outstanding_disk_writes;
    
    /* The number of currently active write transactions */
    int active_write_transactions;
    
    bool writeback_in_progress;
    unsigned int active_flushes;

    bool force_diff_storage_flush;

    cache_t *cache;

    /* The flush lock is necessary because if we acquire dirty blocks
     * in random order during the flush, there might be a deadlock
     * with set_fsm. When the flush is initiated, the flush_lock is
     * grabbed, all write transactions are drained, and no new write transactions
     * are allowed in the meantime. Once all transactions are drained,
     * the writeback code locks all dirty blocks for reading and
     * releases the flush lock. */
    
    /* Note that the write transactions lock the flush lock for reading, because there can be more
    than one write transaction proceeding at the same time (not on the same bufs, but the bufs'
    locks will take care of that) while the writeback locks the flush lock for writing, because it
    must exclude all of the transactions.
    */
    
    rwi_lock_t flush_lock;
    
    // List of things waiting for their data to be written to disk. They will be called back after
    // the next complete writeback cycle completes.
    intrusive_list_t<sync_callback_t> sync_callbacks;
    
    // If something requests a sync but a sync is already in progress, then
    // start_next_sync_immediately is set so that a new sync operation is started as soon as the
    // old one finishes. Note that start_next_sync_immediately being true is not equivalent to
    // sync_callbacks being nonempty, because when wait_for_flush is set, transactions will sit
    // patiently in sync_callbacks without setting start_next_sync_immediately.
    bool start_next_sync_immediately;
    
    // List of bufs that are currenty dirty
    intrusive_list_t<local_buf_t> dirty_bufs;

    ticks_t start_time;

public:
    // A separate type to support concurrent flushes
    class concurrent_flush_t :
            public serializer_t::write_txn_callback_t,
            public lock_available_callback_t {

    protected:
        friend class writeback_t;
        // Please note: constructing triggers flushing
        concurrent_flush_t(writeback_t* parent);

    private:
        virtual ~concurrent_flush_t() { }

        /* Functions and callbacks for different phases of the writeback */
        void start_and_acquire_lock();   // Called on cache thread
        void prepare_patches(); // Called on cache thread
        void do_writeback();  // Called on cache thread
        virtual void on_lock_available();   // Called on cache thread
        void acquire_bufs();   // Called on cache thread
        bool do_write(const bool write_issued);   // Called on serializer thread
        virtual void on_serializer_write_txn();   // Called on serializer thread
        bool do_cleanup();   // Called on cache thread

        writeback_t* parent; // We need this for flush concurrency control (i.e. flush_lock, active_flushes etc.)

        ticks_t start_time;
        ticks_t start_time2;

        // Callbacks for the current sync
        intrusive_list_t<sync_callback_t> current_sync_callbacks;

        // Transaction that the writeback is using to grab buffers
        transaction_t *transaction;

        // Transaction to submit to the serializer
        std::vector<translator_serializer_t::write_t> serializer_writes;

        // We need these to update the transaction ids after issuing a serializer write
        std::vector<inner_buf_t*> serializer_inner_bufs;
    };
};

#endif // __BUFFER_CACHE_WRITEBACK_HPP__

