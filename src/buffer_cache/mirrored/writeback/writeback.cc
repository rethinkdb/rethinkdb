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
    pm_flushes_diff_flush("flushes_diff_flushing", secs_to_ticks(1)),
    pm_flushes_diff_store("flushes_diff_store", secs_to_ticks(1)),
    pm_flushes_locking("flushes_locking", secs_to_ticks(1)),
    pm_flushes_writing("flushes_writing", secs_to_ticks(1));

writeback_t::writeback_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold,
        unsigned int max_dirty_blocks,
        unsigned int flush_waiting_threshold,
        unsigned int max_concurrent_flushes
        )
    : wait_for_flush(wait_for_flush),
      flush_waiting_threshold(flush_waiting_threshold),
      max_concurrent_flushes(max_concurrent_flushes),
      max_dirty_blocks(max_dirty_blocks),
      flush_time_randomizer(flush_timer_ms),
      flush_threshold(flush_threshold),
      flush_timer(NULL),
      outstanding_disk_writes(0),
      active_write_transactions(0),
      writeback_in_progress(false),
      active_flushes(0),
      force_diff_storage_flush(false),
      cache(cache),
      start_next_sync_immediately(false) {
}

writeback_t::~writeback_t() {
    rassert(!flush_timer);
    rassert(outstanding_disk_writes == 0);
    rassert(active_write_transactions == 0);
}

bool writeback_t::sync(sync_callback_t *callback) {
    cache->assert_thread();

    if (num_dirty_blocks() == 0 && sync_callbacks.size() == 0)
        return true;

    if (callback)
        sync_callbacks.push_back(callback);

    if (!writeback_in_progress && active_flushes < max_concurrent_flushes) {
        /* Start the writeback process immediately */
        new concurrent_flush_t(this);
        return false;
    } else {
        /* There is a writeback currently in progress, but sync() has been called, so there is
        more data that needs to be flushed that didn't become part of the current sync. So we
        start another sync right after this one. */
        start_next_sync_immediately = true;
        return false;
    }
}

bool writeback_t::sync_patiently(sync_callback_t *callback) {
    if (callback)
        sync_callbacks.push_back(callback);

    return false;
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
        } else if (sync_callbacks.size() >= flush_waiting_threshold) {
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

void writeback_t::concurrent_flush_t::start_and_acquire_lock() {
    pm_flushes_locking.begin(&start_time);
    parent->cache->assert_thread();
        
    // Cancel the flush timer because we're doing writeback now, so we don't need it to remind
    // us later. This happens only if the flush timer is running, and writeback starts for some
    // other reason before the flush timer goes off; if this writeback had been started by the
    // flush timer, then flush_timer would be NULL here, because flush_timer_callback sets it
    // to NULL.
    if (parent->flush_timer) {
        cancel_timer(parent->flush_timer);
        parent->flush_timer = NULL;
    }

    pm_flushes_diff_flush.begin(&start_time2);
    // As we cannot afford waiting for blocks to get loaded from disk while holding the flush lock
    // we instead try to reclaim some space in the on-disk diff storage now.
    // (we only do this occasionally, hoping that most of the time a compression of the log will do the trick)
    // TODO!
    if (parent->force_diff_storage_flush || randint(800) < (int)parent->dirty_bufs.size()) {
        parent->cache->diff_oocore_storage.flush_n_oldest_blocks(parent->cache->diff_oocore_storage.get_number_of_log_blocks() / (parent->force_diff_storage_flush ? 20 : 100) + 1);
        parent->force_diff_storage_flush = false;
    }
    //if (randint(10) == 0) {
    //    parent->cache->diff_oocore_storage.compress_n_oldest_blocks(parent->cache->diff_oocore_storage.get_number_of_log_blocks() / 5);
    //}
    pm_flushes_diff_flush.end(&start_time2);

    /* Start a read transaction so we can request bufs. */
    rassert(transaction == NULL);
    if (parent->cache->state == cache_t::state_shutting_down_start_flush ||
        parent->cache->state == cache_t::state_shutting_down_waiting_for_flush) {
        // Backdoor around "no new transactions" assert.
        parent->cache->shutdown_transaction_backdoor = true;
    }
    transaction = parent->cache->begin_transaction(rwi_read, NULL);
    parent->cache->shutdown_transaction_backdoor = false;
    rassert(transaction != NULL); // Read txns always start immediately.

    /* Request exclusive flush_lock, forcing all write txns to complete. */
    if (parent->flush_lock.lock(rwi_write, this))
        on_lock_available(); // Continue immediately
}

void writeback_t::concurrent_flush_t::on_lock_available() {
    rassert(parent->writeback_in_progress);
    do_writeback();
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

writeback_t::concurrent_flush_t::concurrent_flush_t(writeback_t* parent) :
        parent(parent) {
    transaction = NULL;

    // Start flushing immediately

    if (parent->dirty_bufs.size() == 0 && parent->sync_callbacks.size() == 0) {
        delete this;
        return;
    }

    rassert(!parent->writeback_in_progress);
    parent->writeback_in_progress = true;
    ++parent->active_flushes;

    coro_t::spawn(&writeback_t::concurrent_flush_t::start_and_acquire_lock, this);
}

void writeback_t::concurrent_flush_t::do_writeback() {
    rassert(parent->writeback_in_progress);
    parent->cache->assert_thread();

    pm_flushes_locking.end(&start_time);

    // Go through the different flushing steps...
    prepare_patches();
    acquire_bufs();

    // Write transactions can now proceed again.
    parent->flush_lock.unlock();

    do_on_thread(parent->cache->serializer->home_thread, this, &writeback_t::concurrent_flush_t::do_write, false);
    // ... continue in on_serializer_write_txn
}

perfmon_sampler_t pm_flushes_blocks("flushes_blocks", secs_to_ticks(1), true),
        pm_flushes_blocks_dirty("flushes_blocks_need_flush", secs_to_ticks(1), true),
        pm_flushes_diff_patches_stored("flushes_diff_patches_stored", secs_to_ticks(1), true);

perfmon_sampler_t pm_flushes_diff_storage_failures("flushes_diff_storage_failures", secs_to_ticks(1));

void writeback_t::concurrent_flush_t::prepare_patches() {
    parent->cache->assert_thread();
    rassert(parent->writeback_in_progress);

    /* Write patches for blocks we don't want to flush now */
    // Please note: Writing patches to the oocore_storage can still alter the dirty_bufs list!
    pm_flushes_diff_store.begin(&start_time2);
    bool diff_storage_failure = false;
    unsigned int patches_stored = 0;
    for (intrusive_list_t<local_buf_t>::iterator lbuf_it = parent->dirty_bufs.begin(); lbuf_it != parent->dirty_bufs.end(); lbuf_it++) {
        local_buf_t *lbuf = &(*lbuf_it);
        inner_buf_t *inner_buf = lbuf->gbuf;

        const ser_transaction_id_t transaction_id = inner_buf->transaction_id;

        if (!lbuf->needs_flush) {
            const std::vector<buf_patch_t*>* patches = parent->cache->diff_core_storage.get_patches(inner_buf->block_id);
            if (patches != NULL) {
#ifndef NDEBUG
                patch_counter_t previous_patch_counter = 0;
#endif
                const std::vector<buf_patch_t*>::const_iterator patches_end = patches->end();
                for (std::vector<buf_patch_t*>::const_iterator patch = patches->begin(); patch != patches_end; ++patch) {
                    rassert(transaction_id > NULL_SER_TRANSACTION_ID);
                    rassert((*patch)->get_patch_counter() == previous_patch_counter + 1);
                    if (lbuf->last_patch_materialized < (*patch)->get_patch_counter()) {
                        if (diff_storage_failure || !parent->cache->diff_oocore_storage.store_patch(**patch, transaction_id)) {
                            lbuf->needs_flush = true;
                            // We don't need the patches anymore
                            parent->cache->diff_core_storage.drop_patches(inner_buf->block_id);
                            diff_storage_failure = true;
                            break;
                        }
                        else {
                            patches_stored++;
                            lbuf->last_patch_materialized = (*patch)->get_patch_counter();
                        }
                    }
#ifndef NDEBUG
                    previous_patch_counter = (*patch)->get_patch_counter();
#endif
                }
            }
        }

        if (lbuf->needs_flush) {
            inner_buf->next_patch_counter = 1;
            lbuf->last_patch_materialized = 0;
            rassert (!parent->cache->diff_core_storage.get_patches(inner_buf->block_id));
        }
    }
    pm_flushes_diff_store.end(&start_time2);
    if (diff_storage_failure)
        parent->force_diff_storage_flush = true; // Make sure we solve the problem for the next flush...

    pm_flushes_diff_patches_stored.record(patches_stored);
    if (diff_storage_failure)
        pm_flushes_diff_storage_failures.record(patches_stored); // TODO! (this does not do what I expected)
}

void writeback_t::concurrent_flush_t::acquire_bufs() {
    parent->cache->assert_thread();
    
    // Move callbacks to locals only after we got the lock.
    // That way callbacks coming in while waiting for the flush lock
    // can still go into this flush.
    current_sync_callbacks.append_and_clear(&parent->sync_callbacks);
    
    // Also, at this point we can still clear the start_next_sync_immediately...
    parent->start_next_sync_immediately = false;    
    
    /* Request read locks on all of the blocks we need to flush. */
    pm_flushes_writing.begin(&start_time);

    serializer_writes.reserve(parent->dirty_bufs.size());
    serializer_inner_bufs.reserve(parent->dirty_bufs.size());

    // Log the size of this flush
    pm_flushes_blocks.record(parent->dirty_bufs.size());

    unsigned int really_dirty = 0;

    int i = 0;
    while (local_buf_t *lbuf = parent->dirty_bufs.head()) {
        inner_buf_t *inner_buf = lbuf->gbuf;
        serializer_inner_bufs.push_back(inner_buf);

        bool buf_needs_flush = lbuf->needs_flush;
        //bool buf_dirty = lbuf->dirty;
        bool recency_dirty = lbuf->recency_dirty;

        // Removes it from dirty_bufs
        lbuf->set_dirty(false);
        lbuf->set_recency_dirty(false);
        lbuf->needs_flush = false;

        if (buf_needs_flush) {
            ++really_dirty;

            bool do_delete = inner_buf->do_delete;

            // Acquire the blocks
            inner_buf->do_delete = false; /* Backdoor around acquire()'s assertion */
            access_t buf_access_mode = do_delete ? rwi_read : rwi_read_outdated_ok;
            buf_t *buf = transaction->acquire(inner_buf->block_id, buf_access_mode, NULL);
            rassert(buf);         // Acquire must succeed since we hold the flush_lock.

            // Fill the serializer structure
            if (!do_delete) {
                serializer_writes.push_back(translator_serializer_t::write_t::make(
                    inner_buf->block_id,
                    inner_buf->subtree_recency,
                    buf->get_data_read(),
                    new buf_writer_t(parent, buf)
                    ));

            } else {
                // NULL indicates a deletion
                serializer_writes.push_back(translator_serializer_t::write_t::make(
                    inner_buf->block_id,
                    inner_buf->subtree_recency,
                    NULL,
                    NULL
                    ));

                rassert(buf_access_mode != rwi_read_outdated_ok);
                buf->release();

                parent->cache->free_list.release_block_id(inner_buf->block_id);

                delete inner_buf;
            }
        } else if (recency_dirty) {
            // No need to acquire the block.
            serializer_writes.push_back(translator_serializer_t::write_t::make_touch(inner_buf->block_id, inner_buf->subtree_recency, NULL));
        }

        i++;
    }

    pm_flushes_blocks_dirty.record(really_dirty);
}

bool writeback_t::concurrent_flush_t::do_write(const bool write_issued) {
    if (!write_issued) {
        /* Start writing all the dirty bufs down, as a transaction. */

        // TODO(NNW): Now that the serializer/aio-system breaks writes up into
        // chunks, we may want to worry about submitting more heavily contended
        // bufs earlier in the process so more write FSMs can proceed sooner.

        rassert(parent->writeback_in_progress);
        parent->cache->serializer->assert_thread();

        bool continue_instantly = serializer_writes.empty() ||
                parent->cache->serializer->do_write(serializer_writes.data(), serializer_writes.size(), this);

        do_on_thread(parent->cache->home_thread, this, &writeback_t::concurrent_flush_t::do_write, true);

        if (continue_instantly)
            on_serializer_write_txn();

        return true;
    }
    else {
        rassert(parent->writeback_in_progress);
        parent->cache->assert_thread();

        // Retrieve changed transaction ids in case COW was used
        rassert(serializer_inner_bufs.size() == serializer_writes.size());
        for (size_t i = 0; i < serializer_inner_bufs.size(); ++i) {
            if (serializer_writes[i].buf_specified && serializer_writes[i].buf) {
                buf_data_t *ser_data = (buf_data_t*)serializer_writes[i].buf;
                ser_data--;

                serializer_inner_bufs[i]->transaction_id = ser_data->transaction_id;
            }
        }

        // Allow new concurrent flushes to start from now on
        // (we had to wait until the transaction got passed to the serializer)
        parent->writeback_in_progress = false;

        // Also start the next sync now in case it was requested
        if (parent->start_next_sync_immediately) {
            parent->start_next_sync_immediately = false;
            parent->sync(NULL);
        }

        return true;
    }
}

void writeback_t::concurrent_flush_t::on_serializer_write_txn() {
    do_on_thread(parent->cache->home_thread, this, &writeback_t::concurrent_flush_t::do_cleanup);
}

bool writeback_t::concurrent_flush_t::do_cleanup() {
    parent->cache->assert_thread();
    
    /* We are done writing all of the buffers */
    
    bool committed __attribute__((unused)) = transaction->commit(NULL);
    rassert(committed); // Read-only transactions commit immediately.

    while (!current_sync_callbacks.empty()) {
        sync_callback_t *cb = current_sync_callbacks.head();
        current_sync_callbacks.remove(cb);
        cb->on_sync();
    }
    
    pm_flushes_writing.end(&start_time);

    --parent->active_flushes;
    
    if (parent->start_next_sync_immediately) {
        parent->start_next_sync_immediately = false;
        parent->sync(NULL);
    }

    delete this;
    return true;
}

