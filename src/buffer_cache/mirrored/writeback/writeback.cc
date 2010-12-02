#include "writeback.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

perfmon_duration_sampler_t
    pm_flushes_locking("flushes_locking", secs_to_ticks(1)),
    pm_flushes_writing("flushes_writing", secs_to_ticks(1));

writeback_t::writeback_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold)
    : wait_for_flush(wait_for_flush),
      flush_time_randomizer(flush_timer_ms),
      flush_threshold(flush_threshold),
      flush_timer(NULL),
      writeback_in_progress(false),
      cache(cache),
      start_next_sync_immediately(false),
      transaction(NULL) {
}

writeback_t::~writeback_t() {
    assert(!flush_timer);
}

bool writeback_t::sync(sync_callback_t *callback) {

    if (!writeback_in_progress) {
        /* Start the writeback process immediately */
        if (writeback_start_and_acquire_lock()) {
            return true;
        } else {
            /* Wait until here before putting the callback into the callbacks list, because we
            don't want to call the callback if the writeback completes immediately. */
            if (callback) current_sync_callbacks.push_back(callback);
            return false;
        }
    
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

    if (num_dirty_blocks() > 0) {
        if (callback) sync_callbacks.push_back(callback);
        return false;
        
    } else {
        // There's nothing to flush, so the sync is "done"
        return true;
    }
}

bool writeback_t::begin_transaction(transaction_t *txn,
        transaction_begin_callback_t *callback) {
    
    switch(txn->get_access()) {
        case rwi_read:
            return true;
        case rwi_write:
            // Lock the flush lock "for reading", but what we really mean is to lock it non-
            // exclusively because more than one write transaction can proceed at once.
            if (flush_lock.lock(rwi_read, txn)) {
                return true;
            } else {
                txn->begin_callback = callback;
                return false;
            }
        default:
            fail("Transaction access invalid.");
    }
}

void writeback_t::on_transaction_commit(transaction_t *txn) {
    
    if (txn->get_access() == rwi_write) {
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

void writeback_t::local_buf_t::set_dirty(bool _dirty) {
    if(!dirty && _dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        gbuf->cache->writeback.dirty_bufs.push_back(this);
        pm_n_blocks_dirty++;
    }
    if(dirty && !_dirty) {
        // We need to "unmark" the buf
        dirty = false;
        gbuf->cache->writeback.dirty_bufs.remove(this);
        pm_n_blocks_dirty--;
    }
}

void writeback_t::flush_timer_callback(void *ctx) {
    writeback_t *self = static_cast<writeback_t *>(ctx);
    self->flush_timer = NULL;
    
    /* Don't sync if we're in the shutdown process, because if we do that we'll trip an assert() on
    the cache, and besides we're about to sync anyway. */
    if (self->cache->state != cache_t::state_shutting_down_waiting_for_transactions) {
        self->sync(NULL);
    }
}

bool writeback_t::writeback_start_and_acquire_lock() {

    assert(!writeback_in_progress);
    writeback_in_progress = true;
    pm_flushes_locking.begin(&start_time);
    cache->assert_cpu();
        
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
    assert(transaction == NULL);
    if (cache->state == cache_t::state_shutting_down_start_flush ||
        cache->state == cache_t::state_shutting_down_waiting_for_flush) {
        // Backdoor around "no new transactions" assert.
        cache->shutdown_transaction_backdoor = true;
    }
    transaction = cache->begin_transaction(rwi_read, NULL);
    cache->shutdown_transaction_backdoor = false;
    assert(transaction != NULL); // Read txns always start immediately.

    /* Request exclusive flush_lock, forcing all write txns to complete. */
    if (flush_lock.lock(rwi_write, this)) return writeback_acquire_bufs();
    else return false;
}

void writeback_t::on_lock_available() {

    assert(writeback_in_progress);
    writeback_acquire_bufs();
}

struct buf_writer_t :
    public serializer_t::write_block_callback_t,
    public cpu_message_t,
    public home_cpu_mixin_t
{
    mc_buf_t *buf;
    buf_writer_t(mc_buf_t *buf) : buf(buf) { }
    void on_serializer_write_block() {
        if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
    }
    void on_cpu_switch() {
        buf->release();
        delete this;
    }
};

bool writeback_t::writeback_acquire_bufs() {
    
    assert(writeback_in_progress);
    cache->assert_cpu();
    
    pm_flushes_locking.end(&start_time);
    pm_flushes_writing.begin(&start_time);
    
    current_sync_callbacks.append_and_clear(&sync_callbacks);

    /* Request read locks on all of the blocks we need to flush.
    TODO: Find a better way to do the allocation. */
    num_serializer_writes = dirty_bufs.size();
    serializer_writes = new serializer_t::write_t[num_serializer_writes];
    
    int i = 0;
    while (local_buf_t *lbuf = dirty_bufs.head()) {
    
        inner_buf_t *inner_buf = lbuf->gbuf;
        lbuf->set_dirty(false);   // Removes it from dirty_bufs
        
        bool do_delete = inner_buf->do_delete;
        
        // Acquire the blocks
        inner_buf->do_delete = false; /* Backdoor around acquire()'s assertion */
        access_t buf_access_mode = do_delete ? rwi_read : rwi_read_outdated_ok;
        buf_t *buf = transaction->acquire(inner_buf->block_id, buf_access_mode, NULL);
        assert(buf);         // Acquire must succeed since we hold the flush_lock.
        
        serializer_writes[i].block_id = inner_buf->block_id;
        
        // Fill the serializer structure
        if (!do_delete) {
        
            serializer_writes[i].buf = buf->get_data_read();
            serializer_writes[i].callback = new buf_writer_t(buf);

        } else {
        
            serializer_writes[i].buf = NULL;   // NULL indicates a deletion
            serializer_writes[i].callback = NULL;
            
            assert(buf_access_mode != rwi_read_outdated_ok);
            buf->release();
            
            cache->free_list.release_block_id(inner_buf->block_id);
            
            delete inner_buf;
        }
        
        i++;
    }
    
    flush_lock.unlock(); // Write transactions can now proceed again.
    
    return do_on_cpu(cache->serializer->home_cpu, this, &writeback_t::writeback_do_write);
}

bool writeback_t::writeback_do_write() {

    /* Start writing all the dirty bufs down, as a transaction. */
    
    // TODO(NNW): Now that the serializer/aio-system breaks writes up into
    // chunks, we may want to worry about submitting more heavily contended
    // bufs earlier in the process so more write FSMs can proceed sooner.
    
    assert(writeback_in_progress);
    cache->serializer->assert_cpu();
    
    if (num_serializer_writes == 0 ||
            cache->serializer->do_write(serializer_writes, num_serializer_writes, this)) {
        return do_on_cpu(cache->home_cpu, this, &writeback_t::writeback_do_cleanup);
    } else {
        return false;
    }
}

void writeback_t::on_serializer_write_txn() {
    
    assert(writeback_in_progress);
    cache->serializer->assert_cpu();
    do_on_cpu(cache->home_cpu, this, &writeback_t::writeback_do_cleanup);
}

bool writeback_t::writeback_do_cleanup() {
    
    assert(writeback_in_progress);
    cache->assert_cpu();
    
    /* We are done writing all of the buffers */
    
    bool committed __attribute__((unused)) = transaction->commit(NULL);
    assert(committed); // Read-only transactions commit immediately.
    transaction = NULL;
    
    delete[] serializer_writes;
    
    while (!current_sync_callbacks.empty()) {
        sync_callback_t *cb = current_sync_callbacks.head();
        current_sync_callbacks.remove(cb);
        cb->on_sync();
    }
    
    // Don't clear writeback_in_progress until after we call all the sync callbacks, because
    // otherwise it would crash if a sync callback called sync().
    writeback_in_progress = false;
    pm_flushes_writing.end(&start_time);

    if (start_next_sync_immediately) {
        start_next_sync_immediately = false;
        sync(NULL);
    }
    
    return true;
}

