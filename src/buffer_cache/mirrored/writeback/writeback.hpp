#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include <set>

#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/mirrored/writeback/flush_time_randomizer.hpp"
#include "utils.hpp"
#include "serializer/serializer.hpp"
#include "buffer_cache/buf_patch.hpp"

class mc_cache_t;
class mc_buf_t;
class mc_inner_buf_t;
class mc_transaction_t;

class writeback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_transaction_t transaction_t;
    
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

    /* `begin_transaction()` will block if the transaction is a write transaction and
    it ought to be throttled */
    void begin_transaction(transaction_t *txn);

    void on_transaction_commit(transaction_t *txn);

    unsigned int num_dirty_blocks() {
        return dirty_bufs.size();
    }

    class local_buf_t : public intrusive_list_node_t<local_buf_t> {
        friend class writeback_t;
        friend class concurrent_flush_t;
        
    public:
        explicit local_buf_t(inner_buf_t *gbuf)
            : needs_flush(false), last_patch_materialized(0), gbuf(gbuf), dirty(false), recency_dirty(false) {}
        
        void set_dirty(bool _dirty = true);
        void set_recency_dirty(bool _recency_dirty = true);
        void mark_block_id_deleted();
        
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
    unsigned int max_dirty_blocks;
    
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

    bool writeback_in_progress;
    unsigned int active_flushes;

    /* Use `adjustable_semaphore_t` instead of `semaphore_t` so we can get `force_lock()`. */
    adjustable_semaphore_t dirty_block_semaphore;

    bool force_patch_storage_flush;

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

    // List of block_ids that have been deleted
    struct deleted_block_t {
        block_id_t block_id;
        bool write_empty_block;
    };
    std::vector<deleted_block_t> deleted_blocks;

    // List of block ids which have to be rejected when offered as part of read ahead.
    // Specifically, blocks which have been deleted but which deletion has not been
    // passed on to the serializer yet are listed here.
    std::set<block_id_t> reject_read_ahead_blocks;

    /* Internal variables used only during a flush operation. */

    ticks_t start_time;         // TODO (rntz) remove if unnecessary

public:
    // This just considers objections which writeback knows about Other subsystems
    // might have their own objections to accepting a read-ahead block...
    // (mc_cache_t::can_read_ahead_block_be_accepted() should consider everything)
    bool can_read_ahead_block_be_accepted(block_id_t block_id);

    // Concurrent flush helpers
    struct buf_writer_t;      // public so that mc_buf_t can declare it a friend

private:
    struct flush_state_t;
    void start_concurrent_flush();
    void do_concurrent_flush();
    void flush_prepare_patches();
    void flush_acquire_bufs(transaction_t *transaction, flush_state_t &state);
    void flush_update_block_sequence_ids(flush_state_t &state);
};

#endif // __BUFFER_CACHE_WRITEBACK_HPP__

