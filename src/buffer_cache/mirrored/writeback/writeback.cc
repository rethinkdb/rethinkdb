#include "writeback.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

writeback_t::begin_transaction_fsm_t::begin_transaction_fsm_t(writeback_t *wb, mc_transaction_t *txn, mc_transaction_begin_callback_t *cb)
    : writeback(wb), transaction(txn)
{
    txn->begin_callback = cb;
    if (writeback->too_many_dirty_blocks()) {
        /* When a flush happens, we will get popped off the throttled transactions list
        and given a green light. */
        writeback->throttled_transactions_list.push_back(this);
    } else {
        green_light();
    }
}

void writeback_t::begin_transaction_fsm_t::green_light() {

    writeback->active_write_transactions++;

    // Lock the flush lock "for reading", but what we really mean is to lock it non-
    // exclusively because more than one write transaction can proceed at once.
    if (writeback->flush_lock.lock(rwi_read, this)) {
        /* push callback on the global event queue - if the lock
         * returns true, then the request won't have been put on the
         * flush lock's queue, which can lead to the
         * transaction_begin_callback not being called when it should be
         * in certain cases, as shown by append_stress append
         * reorderings.
         */
        call_later_on_this_thread(this);
    }
}

void writeback_t::begin_transaction_fsm_t::on_thread_switch() {
    on_lock_available();
}

void writeback_t::begin_transaction_fsm_t::on_lock_available() {
    transaction->green_light();
    delete this;
}

perfmon_duration_sampler_t
    pm_flushes_locking("flushes_locking", secs_to_ticks(1)),
    pm_flushes_writing("flushes_writing", secs_to_ticks(1));

writeback_t::writeback_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold,
        unsigned int max_dirty_blocks
        )
    : wait_for_flush(wait_for_flush),
      max_dirty_blocks(max_dirty_blocks),
      flush_time_randomizer(flush_timer_ms),
      flush_threshold(flush_threshold),
      flush_timer(NULL),
      outstanding_disk_writes(0),
      active_write_transactions(0),
      writeback_in_progress(false),
      cache(cache),
      start_next_sync_immediately(false),
      transaction(NULL) {
      concurrent_flushing = wait_for_flush; // Enable concurrent flushing if transactions will be waiting for us
}

writeback_t::~writeback_t() {
    rassert(!flush_timer);
    rassert(outstanding_disk_writes == 0);
    rassert(active_write_transactions == 0);
}

bool writeback_t::sync(sync_callback_t *callback) {
    if (!writeback_in_progress) {
        /* Start the writeback process immediately */
        if (callback)
            sync_callbacks.push_back(callback);
        writeback_start_and_acquire_lock();
        return false;
    } else {
        /* There is a writeback currently in progress, but sync() has been called, so there is
        more data that needs to be flushed that didn't become part of the current sync. So we
        start another sync right after this one. */
        // TODO: If we haven't acquired the flush lock yet, we could join the current writeback
        // rather than wait for the next one.
        start_next_sync_immediately = true;
        if (callback) sync_callbacks.push_back(callback);
        return false;
    }
}

bool writeback_t::sync_patiently(sync_callback_t *callback) {
    // TODO! Make configurable
    // TODO! Find a better name
    const unsigned int flush_interval = 8;

    if (num_dirty_blocks() > 0) {                
        if (callback)
            sync_callbacks.push_back(callback);

        if (sync_callbacks.size() >= flush_interval) {
            if (writeback_in_progress)
                start_next_sync_immediately = true;
            else
                writeback_start_and_acquire_lock();
        }

        return false;
    } else {
        // There's nothing to flush, so the sync is "done"
        return true;
    }
}

bool writeback_t::begin_transaction(transaction_t *txn, transaction_begin_callback_t *callback) {
    switch (txn->get_access()) {
        case rwi_read:
            return true;
        case rwi_write:
            new begin_transaction_fsm_t(this, txn, callback);
            return false;
        case rwi_read_outdated_ok:
        case rwi_intent:
        case rwi_upgrade:
        default:
            unreachable("Transaction access invalid.");
    }
}

void writeback_t::on_transaction_commit(transaction_t *txn) {
    if (txn->get_access() == rwi_write) {
        
        active_write_transactions--;
        possibly_unthrottle_transactions();
        
        flush_lock.unlock();
        
        /* At the end of every write transaction, check if the number of dirty blocks exceeds the
        threshold to force writeback to start. */
        if (num_dirty_blocks() > flush_threshold) {
            sync(NULL);
        } else if (num_dirty_blocks() > 0 && flush_time_randomizer.is_zero()) {
            sync(NULL);
        } else if (!flush_timer && !flush_time_randomizer.is_never_flush() && num_dirty_blocks() > 0) {
            /* Start the flush timer so that the modified data doesn't sit in memory for too long
            without being written to disk. */
            flush_timer = fire_timer_once(flush_time_randomizer.next_time_interval(), flush_timer_callback, this);
        }
    }
}

unsigned int writeback_t::num_dirty_blocks() {
    return dirty_bufs.size();
}

bool writeback_t::too_many_dirty_blocks() {
    return
        num_dirty_blocks() + outstanding_disk_writes + active_write_transactions * 3
        >= max_dirty_blocks;
}

void writeback_t::possibly_unthrottle_transactions() {
    while (!too_many_dirty_blocks()) {
        begin_transaction_fsm_t *fsm = throttled_transactions_list.head();
        if (!fsm) return;
        throttled_transactions_list.remove(fsm);
        fsm->green_light();

        /* green_light() should never have an immediate effect; the most it can do is push the
        transaction onto the event queue. */
        rassert(!too_many_dirty_blocks());
    }
}

void writeback_t::local_buf_t::set_dirty(bool _dirty) {
    if (!dirty && _dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.push_back(this);
        }
        pm_n_blocks_dirty++;
    }
    if (dirty && !_dirty) {
        // We need to "unmark" the buf
        dirty = false;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.remove(this);
            gbuf->cache->writeback.possibly_unthrottle_transactions();
        }
        pm_n_blocks_dirty--;
    }
}

void writeback_t::local_buf_t::set_recency_dirty(bool _recency_dirty) {
    if (!recency_dirty && _recency_dirty) {
        // Mark block as recency_dirty if it hasn't been already.
        recency_dirty = true;
        if (!dirty) {
            gbuf->cache->writeback.dirty_bufs.push_back(this);
        }
        // TODO perfmon
    }
    if (recency_dirty && !_recency_dirty) {
        recency_dirty = false;
        if (!dirty) {
            gbuf->cache->writeback.dirty_bufs.remove(this);
            gbuf->cache->writeback.possibly_unthrottle_transactions();
        }
        // TODO perfmon
    }
}

void writeback_t::flush_timer_callback(void *ctx) {
    writeback_t *self = static_cast<writeback_t *>(ctx);
    self->flush_timer = NULL;
    
    /* Don't sync if we're in the shutdown process, because if we do that we'll trip an rassert() on
    the cache, and besides we're about to sync anyway. */
    if (self->cache->state != cache_t::state_shutting_down_waiting_for_transactions) {
        self->sync(NULL);
    }
}

void writeback_t::writeback_start_and_acquire_lock() {
    rassert(!writeback_in_progress);
    writeback_in_progress = true;
    pm_flushes_locking.begin(&start_time);
    cache->assert_thread();
        
    // Cancel the flush timer because we're doing writeback now, so we don't need it to remind
    // us later. This happens only if the flush timer is running, and writeback starts for some
    // other reason before the flush timer goes off; if this writeback had been started by the
    // flush timer, then flush_timer would be NULL here, because flush_timer_callback sets it
    // to NULL.
    if (flush_timer) {
        cancel_timer(flush_timer);
        flush_timer = NULL;
    }
    
    /* Start a read transaction so we can request bufs. */
    rassert(transaction == NULL);
    if (cache->state == cache_t::state_shutting_down_start_flush ||
        cache->state == cache_t::state_shutting_down_waiting_for_flush) {
        // Backdoor around "no new transactions" assert.
        cache->shutdown_transaction_backdoor = true;
    }
    transaction = cache->begin_transaction(rwi_read, NULL);
    cache->shutdown_transaction_backdoor = false;
    rassert(transaction != NULL); // Read txns always start immediately.

    /* Request exclusive flush_lock, forcing all write txns to complete. */
    if (flush_lock.lock(rwi_write, this))
        on_lock_available(); // Continue immediately
}

void writeback_t::on_lock_available() {
    rassert(writeback_in_progress);
    coro_t::spawn(&writeback_t::writeback_do_writeback_local, this);
}

struct buf_writer_t :
    public serializer_t::write_block_callback_t,
    public thread_message_t,
    public home_thread_mixin_t
{
    writeback_t *parent;
    mc_buf_t *buf;
    explicit buf_writer_t(writeback_t *wb, mc_buf_t *buf)
        : parent(wb), buf(buf)
    {
        parent->outstanding_disk_writes++;
    }
    void on_serializer_write_block() {
        if (continue_on_thread(home_thread, this)) on_thread_switch();
    }
    void on_thread_switch() {
        buf->release();
        parent->outstanding_disk_writes--;
        parent->possibly_unthrottle_transactions();
        delete this;
    }
};

writeback_t::per_flush_locals_t *writeback_t::create_flush_locals() {
    rassert(writeback_in_progress);
    cache->assert_thread();
    
    per_flush_locals_t *flush_locals = new per_flush_locals_t;
    flush_locals->current_sync_callbacks.append_and_clear(&sync_callbacks);
    
    flush_locals->transaction = transaction;
    transaction = NULL;
    
    flush_locals->flusher_coro = coro_t::self();
    
    return flush_locals;
}

void writeback_t::writeback_do_writeback_local() {
    rassert(writeback_in_progress);
    cache->assert_thread();

    pm_flushes_locking.end(&start_time);

    per_flush_locals_t *flush_locals = create_flush_locals();

    // Go through the different flushing steps...
    writeback_acquire_bufs(flush_locals);
    coro_t::spawn(&writeback_t::writeback_do_write, this, flush_locals);
    coro_t::wait(); // Wait until writes have finished
    writeback_do_cleanup(flush_locals);

    delete flush_locals;
}


void writeback_t::writeback_acquire_bufs(per_flush_locals_t *flush_locals) {
    cache->assert_thread();
    
    pm_flushes_writing.begin(&flush_locals->start_time);
    
    // Request read locks on all of the blocks we need to flush.
    flush_locals->serializer_writes.clear();
    flush_locals->serializer_writes.reserve(dirty_bufs.size());

    int i = 0;
    while (local_buf_t *lbuf = dirty_bufs.head()) {
        inner_buf_t *inner_buf = lbuf->gbuf;

        bool buf_dirty = lbuf->dirty;
#ifndef NDEBUG
        bool recency_dirty = lbuf->recency_dirty;
#endif

        // Removes it from dirty_bufs
        lbuf->set_dirty(false);
        lbuf->set_recency_dirty(false);    

        if (buf_dirty) {
            bool do_delete = inner_buf->do_delete;

            // Acquire the blocks
            inner_buf->do_delete = false; /* Backdoor around acquire()'s assertion */
            access_t buf_access_mode = do_delete ? rwi_read : rwi_read_outdated_ok;
            buf_t *buf = flush_locals->transaction->acquire(inner_buf->block_id, buf_access_mode, NULL);
            rassert(buf);         // Acquire must succeed since we hold the flush_lock.

            // Fill the serializer structure
            if (!do_delete) {
                flush_locals->serializer_writes.push_back(translator_serializer_t::write_t::make(
                    inner_buf->block_id,
                    inner_buf->subtree_recency,
                    buf->get_data_read(),
                    new buf_writer_t(this, buf)
                    ));

            } else {
                // NULL indicates a deletion
                flush_locals->serializer_writes.push_back(translator_serializer_t::write_t::make(
                    inner_buf->block_id,
                    inner_buf->subtree_recency,
                    NULL,
                    NULL
                    ));

                rassert(buf_access_mode != rwi_read_outdated_ok);
                buf->release();

                cache->free_list.release_block_id(inner_buf->block_id);

                delete inner_buf;
            }
        } else {
            rassert(recency_dirty);

            // No need to acquire the block.
            flush_locals->serializer_writes.push_back(translator_serializer_t::write_t::make_touch(inner_buf->block_id, inner_buf->subtree_recency, NULL));
        }

        i++;
    }

    flush_lock.unlock(); // Write transactions can now proceed again.
}

void writeback_t::writeback_do_write(per_flush_locals_t *flush_locals) {
    coro_t::move_to_thread(cache->serializer->home_thread);

    /* Start writing all the dirty bufs down, as a transaction. */
    
    // TODO(NNW): Now that the serializer/aio-system breaks writes up into
    // chunks, we may want to worry about submitting more heavily contended
    // bufs earlier in the process so more write FSMs can proceed sooner.
    
    rassert(writeback_in_progress);
    cache->serializer->assert_thread();
    
    if (flush_locals->serializer_writes.empty() ||
        cache->serializer->do_write(flush_locals->serializer_writes.data(), flush_locals->serializer_writes.size(), flush_locals)) {
        
        // We don't need this buffer any more.
        flush_locals->serializer_writes.clear();
        
        flush_locals->flusher_coro->notify();
    }
    
    coro_t::move_to_thread(cache->home_thread);
    
    // If we allow concurrent flushing, allow new flushes to start from now on
    // (we had to wait until the transaction got passed to the serializer)
    if (concurrent_flushing) {
        writeback_in_progress = false;
        
        if (start_next_sync_immediately) {
            start_next_sync_immediately = false;
            sync(NULL);
        }
    }
}

void writeback_t::per_flush_locals_t::on_serializer_write_txn() {
    flusher_coro->notify();
}

void writeback_t::writeback_do_cleanup(per_flush_locals_t *flush_locals) {
    rassert(concurrent_flushing || writeback_in_progress);
    cache->assert_thread();
    
    /* We are done writing all of the buffers */
    
    bool committed __attribute__((unused)) = flush_locals->transaction->commit(NULL);
    rassert(committed); // Read-only transactions commit immediately.

    flush_locals->serializer_writes.clear();

    while (!flush_locals->current_sync_callbacks.empty()) {
        sync_callback_t *cb = flush_locals->current_sync_callbacks.head();
        flush_locals->current_sync_callbacks.remove(cb);
        cb->on_sync();
    }
    
    pm_flushes_writing.end(&flush_locals->start_time);

    if (!concurrent_flushing) {
        writeback_in_progress = false;
        
        if (start_next_sync_immediately) {
            start_next_sync_immediately = false;
            sync(NULL);
        }
    }
}

