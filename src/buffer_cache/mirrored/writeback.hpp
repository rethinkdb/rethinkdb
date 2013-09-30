// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_WRITEBACK_HPP_
#define BUFFER_CACHE_MIRRORED_WRITEBACK_HPP_

#include <set>
#include <vector>

#include "arch/timer.hpp"
#include "buffer_cache/mirrored/flush_time_randomizer.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/semaphore.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

class cond_t;
class timer_token_t;
class mc_cache_t;
class mc_buf_lock_t;
class mc_inner_buf_t;
class mc_transaction_t;

class sync_callback_t : private cond_t, public intrusive_list_node_t<sync_callback_t> {
public:
    // Waits for all the on_syncs!  Calls wait().
    ~sync_callback_t() { wait(); }

    using cond_t::pulse;
};

class writeback_t : private timer_callback_t {
public:
    writeback_t(
        mc_cache_t *cache,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold,
        unsigned int max_dirty_blocks,
        unsigned int max_concurrent_flushes);
    virtual ~writeback_t();

    /* Forces a writeback to happen soon. If there is nothing to write, pulses the callback.
       Otherwise, pulses the callback as soon as the next writeback cycle is over. */
    void sync(sync_callback_t *callback);

    /* Same as sync(), but doesn't hurry up the writeback very much. */
    void sync_patiently(sync_callback_t *callback);

    /* `begin_transaction()` will block if the transaction is a write transaction and
    it ought to be throttled. */
    void begin_transaction(mc_transaction_t *txn);

    void on_transaction_commit(mc_transaction_t *txn);

    unsigned int num_dirty_blocks() {
        return dirty_bufs.size();
    }

    class local_buf_t : public intrusive_list_node_t<local_buf_t> {
    public:
        local_buf_t();

        /* reset() resets the local_buf_t to its factory settings.
        This is usually called once by the constructor.
        However, due to a peculiarity of how snapshots interact with the deletion of blocks
        and the writeback process (as of 11/20/2012), new blocks can sometimes be assigned a block id
        for which a local_buf_t still exists. In that case, the local_buf_t has to be reset ot its initial
        configuration. This process is currently triggered by mc_inner_buf_t::allocate(). */
        void reset();

        void set_dirty(bool _dirty = true);
        bool get_dirty() const { return dirty; }
        void set_recency_dirty(bool _recency_dirty = true);
        bool get_recency_dirty() const { return recency_dirty; }
        void mark_block_id_deleted();

        bool safe_to_unload() const { return !dirty && !recency_dirty; }
        
        scoped_ptr_t<adjustable_semaphore_acq_t> extract_throttling_lock() {
            return std::move(throttling_lock);
        }

    private:
        bool dirty;
        bool recency_dirty;
        scoped_ptr_t<adjustable_semaphore_acq_t> throttling_lock;

    private:
        DISABLE_COPYING(local_buf_t);
    };

    /* User-controlled settings. */

    const unsigned int max_concurrent_flushes;
    const unsigned int max_dirty_blocks;

    bool has_active_flushes() { return active_flushes > 0; }

private:
    flush_time_randomizer_t flush_time_randomizer;
    const unsigned int flush_threshold;   // Number of blocks, not percentage

private:
    friend class buf_writer_t;
    friend class concurrent_flush_t;

    // The writeback system has a mechanism to keep data safe if the server crashes. If modified
    // data sits in memory for longer than flush_timer_ms milliseconds, a writeback will be
    // automatically started to store it on disk. flush_timer is the timer to keep track of how
    // much longer the data can sit in memory.
    timer_token_t *flush_timer;
    // The flush timer callback.
    void on_timer();

    bool writeback_in_progress;
    unsigned int active_flushes;

    adjustable_semaphore_t dirty_block_semaphore;

    mc_cache_t *cache;

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
    // sync_callbacks being nonempty, because when non-null disk_ack_signals are passed,
    // transactions will sit patiently in sync_callbacks without setting
    // start_next_sync_immediately.
    bool start_next_sync_immediately;

    // List of bufs that are currenty dirty
    intrusive_list_t<local_buf_t> dirty_bufs;

    // List of block_ids that have been deleted
    std::vector<block_id_t> deleted_blocks;

    // List of block ids which have to be rejected when offered as part of read ahead.
    // Specifically, blocks which have been deleted but which deletion has not been
    // passed on to the serializer yet are listed here.
    std::set<block_id_t> reject_read_ahead_blocks;

public:
    cond_t *to_pulse_when_last_active_flush_finishes;

public:
    // This just considers objections which writeback knows about Other subsystems
    // might have their own objections to accepting a read-ahead block...
    // (mc_cache_t::can_read_ahead_block_be_accepted() should consider everything)
    bool can_read_ahead_block_be_accepted(block_id_t block_id);

    // Concurrent flush helpers
    class buf_writer_t;      // public so that mc_buf_lock_t can declare it a friend

private:
    struct flush_state_t;
    void start_concurrent_flush();
    void do_concurrent_flush();
    void flush_acquire_bufs(mc_transaction_t *transaction, flush_state_t *state);
};

#endif // BUFFER_CACHE_MIRRORED_WRITEBACK_HPP_

