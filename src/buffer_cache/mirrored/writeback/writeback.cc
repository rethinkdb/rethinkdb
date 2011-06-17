#include "writeback.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include <cmath>
#include <set>

// TODO: We added a writeback->possibly_unthrottle_transactions() call
// in the begin_transaction_fsm_t(..) constructor, where did that get
// merged to now?


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
        ) :
    wait_for_flush(wait_for_flush),
    flush_waiting_threshold(flush_waiting_threshold),
    max_concurrent_flushes(max_concurrent_flushes),
    max_dirty_blocks(max_dirty_blocks),
    flush_time_randomizer(flush_timer_ms),
    flush_threshold(flush_threshold),
    flush_timer(NULL),
    writeback_in_progress(false),
    active_flushes(0),
    dirty_block_semaphore(max_dirty_blocks),
    force_patch_storage_flush(false),
    cache(cache),
    start_next_sync_immediately(false) {

    rassert(max_dirty_blocks >= 10); // sanity check: you really don't want to have less than this.
                                     // 10 is rather arbitrary.
}

writeback_t::~writeback_t() {
    rassert(!writeback_in_progress);
    rassert(active_flushes == 0);
    rassert(sync_callbacks.size() == 0);
    if (flush_timer) {
        cancel_timer(flush_timer);
        flush_timer = NULL;
    }
}

perfmon_sampler_t pm_patches_size_ratio("patches_size_ratio", secs_to_ticks(5), false);

bool writeback_t::sync(sync_callback_t *callback) {
    cache->assert_thread();
    rassert(cache->writebacks_allowed);

    // Have to check active_flushes too, because a return value of true has to guarantee that changes handled
    // by previous flushes are also on disk. If these are still running, we must initiate a new flush
    // even if there are no dirty blocks to make sure that the callbacks get called only
    // after all other flushes have finished (which is at least enforced by the serializer's metablock queue currently)
    if (num_dirty_blocks() == 0 && sync_callbacks.size() == 0 && active_flushes == 0)
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

void writeback_t::begin_transaction(transaction_t *txn) {

    if (txn->get_access() == rwi_write) {

        /* Throttling */
        dirty_block_semaphore.co_lock(txn->expected_change_count);

        /* Acquire flush lock in non-exclusive mode */
        flush_lock.co_lock(rwi_read);
    } else if (txn->get_access() == rwi_read_sync) {

        /* Throttling */
        dirty_block_semaphore.co_lock(1); // This 1 is just a dummy thing, so we go through the throttling queue

        /* Acquire flush lock in non-exclusive mode */
        flush_lock.co_lock(rwi_read);

        // We do three things now:
        // 1. degrade the transaction to a "normal" rwi_read transaction
        // 2, Increase the dirty_block_semaphore again (undo our "dummy" 1)
        // 3. unlock the flush_lock immediately, we don't really need it
        txn->access = rwi_read;

        dirty_block_semaphore.unlock(1);
        flush_lock.unlock();
    }
}

void writeback_t::on_transaction_commit(transaction_t *txn) {
    if (txn->get_access() == rwi_write) {

        dirty_block_semaphore.unlock(txn->expected_change_count);

        flush_lock.unlock();

        /* At the end of every write transaction, check if the number of dirty blocks exceeds the
        threshold to force writeback to start. */
        if (num_dirty_blocks() > flush_threshold) {
            sync(NULL);
        } else if (num_dirty_blocks() > 0 && flush_time_randomizer.is_zero()) {
            sync(NULL);
        } else if (sync_callbacks.size() >= flush_waiting_threshold) {
            sync(NULL);
        }

        if (!flush_timer && !flush_time_randomizer.is_never_flush() && !flush_time_randomizer.is_zero()) {
            /* Start the flush timer so that the modified data doesn't sit in memory for too long
            without being written to disk and the patches_size_ratio gets updated */
            flush_timer = fire_timer_once(flush_time_randomizer.next_time_interval(), flush_timer_callback, this);
        }
    }
}

void writeback_t::local_buf_t::set_dirty(bool _dirty) {
    if (!dirty && _dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.push_back(this);
            /* Use `force_lock()` to prevent deadlocks; `co_lock()` could block. */
            gbuf->cache->writeback.dirty_block_semaphore.force_lock();
        }
        pm_n_blocks_dirty++;
    }
    if (dirty && !_dirty) {
        // We need to "unmark" the buf
        dirty = false;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.remove(this);
            gbuf->cache->writeback.dirty_block_semaphore.unlock();
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
            gbuf->cache->writeback.dirty_block_semaphore.force_lock();
        }
        // TODO perfmon
    }
    if (recency_dirty && !_recency_dirty) {
        recency_dirty = false;
        if (!dirty) {
            gbuf->cache->writeback.dirty_bufs.remove(this);
            gbuf->cache->writeback.dirty_block_semaphore.unlock();
        }
        // TODO perfmon
    }
}

// Add block_id to deleted_blocks list.
void writeback_t::local_buf_t::mark_block_id_deleted() {
    writeback_t::deleted_block_t deleted_block;
    deleted_block.block_id = gbuf->block_id;
    deleted_block.write_empty_block = gbuf->write_empty_deleted_block;
    gbuf->cache->writeback.deleted_blocks.push_back(deleted_block);

    // As the block has been deleted, we must not accept any versions of it offered
    // by a read-ahead operation
    gbuf->cache->writeback.reject_read_ahead_blocks.insert(gbuf->block_id);
}

bool writeback_t::can_read_ahead_block_be_accepted(block_id_t block_id) {
    return reject_read_ahead_blocks.find(block_id) == reject_read_ahead_blocks.end();
}

void writeback_t::flush_timer_callback(void *ctx) {
    writeback_t *self = static_cast<writeback_t *>(ctx);
    self->flush_timer = NULL;

    self->cache->assert_thread();

    pm_patches_size_ratio.record(self->cache->max_patches_size_ratio);

    if (self->active_flushes < self->max_concurrent_flushes || self->num_dirty_blocks() < (float)self->max_dirty_blocks * RAISE_PATCHES_RATIO_AT_FRACTION_OF_UNSAVED_DATA_LIMIT) {
        /* The currently running writeback probably finished on-time. (of we have enough headroom left before hitting the unsaved data limit)
        Adjust max_patches_size_ratio to trade i/o efficiency for CPU cycles */
        if (!self->wait_for_flush)
            self->cache->max_patches_size_ratio = (unsigned int)(0.9f * (float)self->cache->max_patches_size_ratio + 0.1f * (float)MAX_PATCHES_SIZE_RATIO_MIN);
    } else {
        /* The currently running writeback apparently takes too long.
        try to reduce that bottleneck by adjusting max_patches_size_ratio */
        if (!self->wait_for_flush)
            self->cache->max_patches_size_ratio = (unsigned int)(0.9f * (float)self->cache->max_patches_size_ratio + 0.1f * (float)MAX_PATCHES_SIZE_RATIO_MAX);
    }

    /* Don't sync if we're in the shutdown process, because if we do that we'll trip an rassert() on
    the cache, and besides we're about to sync anyway. */
    if (!self->cache->shutting_down && (self->num_dirty_blocks() > 0 || self->sync_callbacks.size() > 0)) {
        self->sync(NULL);
    }
}

void writeback_t::concurrent_flush_t::start_and_acquire_lock() {
    pm_flushes_locking.begin(&start_time);
    parent->cache->assert_thread();

    // As we cannot afford waiting for blocks to get loaded from disk while holding the flush lock,
    // we instead reclaim some space in the on-disk patch storage now.
    ticks_t start_time2;
    pm_flushes_diff_flush.begin(&start_time2);
    unsigned int blocks_to_flush = (unsigned long long)parent->dirty_bufs.size() * 100ll / parent->cache->get_block_size().value() + 1;
    if (parent->force_patch_storage_flush) {
        blocks_to_flush = std::max(parent->cache->patch_disk_storage->get_number_of_log_blocks() / 20 + 1, blocks_to_flush);
        parent->force_patch_storage_flush = false;
    }
    parent->cache->patch_disk_storage->clear_n_oldest_blocks(blocks_to_flush);
    pm_flushes_diff_flush.end(&start_time2);

    /* Start a read transaction so we can request bufs. */
    rassert(transaction == NULL);
    {
        // There _must_ not be waiting in the begin_transaction call
        // because then we could have a race condition with
        // shutting_down.
        ASSERT_NO_CORO_WAITING;

        bool saved_shutting_down = parent->cache->shutting_down;
        parent->cache->shutting_down = false;   // Backdoor around "no new transactions" assert.

        // It's a read transaction, that's why we use repli_timestamp::invalid.
        transaction = new mc_transaction_t(parent->cache, rwi_read);
        parent->cache->shutting_down = saved_shutting_down;
    }

    rassert(transaction != NULL); // Read txns always start immediately.

    /* Request exclusive flush_lock, forcing all write txns to complete. */
    if (parent->flush_lock.lock(rwi_write, this))
        on_lock_available(); // Continue immediately
}

void writeback_t::concurrent_flush_t::on_lock_available() {
    rassert(parent->writeback_in_progress);
    do_writeback();
}

struct writeback_t::concurrent_flush_t::buf_writer_t :
    public serializer_t::write_block_callback_t,
    public thread_message_t,
    public home_thread_mixin_t
{
    writeback_t *parent;
    mc_buf_t *buf;
    bool *transaction_ids_have_been_updated;
    bool released_buffer;
    explicit buf_writer_t(writeback_t *wb, mc_buf_t *buf, bool *tids_updated)
        : parent(wb), buf(buf), transaction_ids_have_been_updated(tids_updated), released_buffer(false)
    {
        /* When we spawn a flush, the block ceases to be dirty, so we release the
        semaphore. To avoid releasing a tidal wave of write transactions every time
        the flush starts, we have the writer acquire the semaphore and release it only
        once the block is safely on disk. */
        parent->dirty_block_semaphore.force_lock(1);
    }
    void on_serializer_write_block() {
        if (continue_on_thread(home_thread(), this)) on_thread_switch();
    }
    void on_thread_switch() {
        parent->dirty_block_semaphore.unlock();
        if (!*transaction_ids_have_been_updated) {
            // Writeback might still need the buffer. We wait until we get destructed before releasing it...
            return;
        }
        released_buffer = true;
        buf->release();
    }
    ~buf_writer_t() {
        if (!released_buffer) {
            buf->release();
        }
    }
};

writeback_t::concurrent_flush_t::concurrent_flush_t(writeback_t* parent) :
        transaction_ids_have_been_updated(false), parent(parent) {
    transaction = NULL;

    // Start flushing immediately

    if (parent->dirty_bufs.size() == 0 && parent->sync_callbacks.size() == 0) {
        delete this;
        return;
    }

    rassert(!parent->writeback_in_progress);
    parent->writeback_in_progress = true;
    ++parent->active_flushes;

    coro_t::spawn(boost::bind(&writeback_t::concurrent_flush_t::start_and_acquire_lock, this));
}

void writeback_t::concurrent_flush_t::do_writeback() {
    rassert(parent->writeback_in_progress);
    parent->cache->assert_thread();

    pm_flushes_locking.end(&start_time);

    // Move callbacks to locals only after we got the lock.
    // That way callbacks coming in while waiting for the flush lock
    // can still go into this flush.
    current_sync_callbacks.append_and_clear(&parent->sync_callbacks);

    // Also, at this point we can still clear the start_next_sync_immediately...
    parent->start_next_sync_immediately = false;

    // Go through the different flushing steps...
    prepare_patches();
    acquire_bufs();

    // Write transactions can now proceed again.
    parent->flush_lock.unlock();

    do_on_thread(parent->cache->serializer->home_thread(), boost::bind(&writeback_t::concurrent_flush_t::do_write, this));
    // ... continue in on_serializer_write_txn
}

perfmon_sampler_t pm_flushes_blocks("flushes_blocks", secs_to_ticks(1), true),
        pm_flushes_blocks_dirty("flushes_blocks_need_flush", secs_to_ticks(1), true),
        pm_flushes_diff_patches_stored("flushes_diff_patches_stored", secs_to_ticks(1), false),
        pm_flushes_diff_storage_failures("flushes_diff_storage_failures", secs_to_ticks(30), true);

void writeback_t::concurrent_flush_t::prepare_patches() {
    parent->cache->assert_thread();
    rassert(parent->writeback_in_progress);

    /* Write patches for blocks we don't want to flush now */
    // Please note: Writing patches to the oocore_storage can still alter the dirty_bufs list!
    ticks_t start_time2;
    pm_flushes_diff_store.begin(&start_time2);
    bool patch_storage_failure = false;
    unsigned int patches_stored = 0;
    for (intrusive_list_t<local_buf_t>::iterator lbuf_it = parent->dirty_bufs.begin(); lbuf_it != parent->dirty_bufs.end(); lbuf_it++) {
        local_buf_t *lbuf = *lbuf_it;
        inner_buf_t *inner_buf = lbuf->gbuf;

        if (patch_storage_failure && lbuf->dirty && !lbuf->needs_flush) {
            lbuf->needs_flush = true;
        }

        if (!lbuf->needs_flush && lbuf->dirty && inner_buf->next_patch_counter > 1) {
            const ser_transaction_id_t transaction_id = inner_buf->transaction_id;
            if (parent->cache->patch_memory_storage.has_patches_for_block(inner_buf->block_id)) {
#ifndef NDEBUG
                patch_counter_t previous_patch_counter = 0;
#endif
                std::pair<patch_memory_storage_t::const_patch_iterator, patch_memory_storage_t::const_patch_iterator>
                    range = parent->cache->patch_memory_storage.patches_for_block(inner_buf->block_id);

                while (range.second != range.first) {
                    --range.second;

                    rassert(transaction_id > NULL_SER_TRANSACTION_ID);
                    rassert(previous_patch_counter == 0 || (*range.second)->get_patch_counter() == previous_patch_counter - 1);
                    if (lbuf->last_patch_materialized < (*range.second)->get_patch_counter()) {
                        if (!parent->cache->patch_disk_storage->store_patch(*(*range.second), transaction_id)) {
                            patch_storage_failure = true;
                            lbuf->needs_flush = true;
                            break;
                        }
                        else {
                            patches_stored++;
                        }
                    }
                    else {
                        break;
                    }
#ifndef NDEBUG
                    previous_patch_counter = (*range.second)->get_patch_counter();
#endif
                }

                if (!patch_storage_failure) {
                    lbuf->last_patch_materialized = parent->cache->patch_memory_storage.last_patch_materialized_or_zero(inner_buf->block_id);
                }
            }
        }

        if (lbuf->needs_flush) {
            inner_buf->next_patch_counter = 1;
            lbuf->last_patch_materialized = 0;
        }
    }
    pm_flushes_diff_store.end(&start_time2);
    if (patch_storage_failure)
        parent->force_patch_storage_flush = true; // Make sure we resolve the storage space shortage for the next flush...

    pm_flushes_diff_patches_stored.record(patches_stored);
    if (patch_storage_failure)
        pm_flushes_diff_storage_failures.record(patches_stored);
}

void writeback_t::concurrent_flush_t::acquire_bufs() {    
    /* Request read locks on all of the blocks we need to flush. */
    pm_flushes_writing.begin(&start_time);

    // Log the size of this flush
    pm_flushes_blocks.record(parent->dirty_bufs.size());

    // Request read locks on all of the blocks we need to flush.
    serializer_writes.reserve(parent->deleted_blocks.size() + parent->dirty_bufs.size());
    serializer_inner_bufs.reserve(parent->dirty_bufs.size());

    // Write deleted block_ids.
    for (size_t i = 0; i < parent->deleted_blocks.size(); i++) {
        // NULL indicates a deletion
        serializer_writes.push_back(translator_serializer_t::write_t::make(
            parent->deleted_blocks[i].block_id,
            repli_timestamp::invalid,
            NULL,
            parent->deleted_blocks[i].write_empty_block,
            NULL
            ));
    }
    parent->deleted_blocks.clear();

    unsigned int really_dirty = 0;

    while (local_buf_t *lbuf = parent->dirty_bufs.head()) {
        inner_buf_t *inner_buf = lbuf->gbuf;

        bool buf_needs_flush = lbuf->needs_flush;
        //bool buf_dirty = lbuf->dirty;
        bool recency_dirty = lbuf->recency_dirty;

        // Removes it from dirty_bufs
        lbuf->set_dirty(false);
        lbuf->set_recency_dirty(false);
        lbuf->needs_flush = false;

        if (buf_needs_flush) {
            ++really_dirty;

            rassert(!inner_buf->do_delete);

            // Acquire the blocks
            buf_t *buf;
            {
                // Acquire always succeeds, but sometimes it blocks.
                // But it won't block because we hold the flush lock.
                ASSERT_NO_CORO_WAITING;
                buf = transaction->acquire(inner_buf->block_id, rwi_read_outdated_ok);
                rassert(buf);
            }

            serializer_inner_bufs.push_back(inner_buf);

            // Fill the serializer structure
            buf_writer_t *buf_writer = new buf_writer_t(parent, buf, &transaction_ids_have_been_updated);
            buf_writers.push_back(buf_writer);
            serializer_writes.push_back(translator_serializer_t::write_t::make(
                inner_buf->block_id,
                inner_buf->subtree_recency,
                buf->get_data_read(),
                inner_buf->write_empty_deleted_block,
                buf_writer));
        } else if (recency_dirty) {
            // No need to acquire the block.
            serializer_writes.push_back(translator_serializer_t::write_t::make_touch(inner_buf->block_id, inner_buf->subtree_recency, NULL));
        }

    }

    pm_flushes_blocks_dirty.record(really_dirty);
}

bool writeback_t::concurrent_flush_t::do_write() {
    /* Start writing all the dirty bufs down, as a transaction. */

    rassert(parent->writeback_in_progress);
    parent->cache->serializer->assert_thread();

    bool continue_instantly = serializer_writes.empty() ||
            parent->cache->serializer->do_write(serializer_writes.data(), serializer_writes.size(), parent->cache->writes_io_account.get(), this, this);

    if (continue_instantly) {
        // the tid_callback gets called even if do_write returns true...
        if (serializer_writes.empty()) {
            do_on_thread(parent->cache->home_thread(), boost::bind(&writeback_t::concurrent_flush_t::update_transaction_ids, this));
        }
        do_on_thread(parent->cache->home_thread(), boost::bind(&writeback_t::concurrent_flush_t::do_cleanup, this));
    }

    return true;
}

void writeback_t::concurrent_flush_t::update_transaction_ids() {
    rassert(parent->writeback_in_progress);
    rassert(!transaction_ids_have_been_updated);
    parent->cache->assert_thread();

    // Retrieve changed transaction ids
    size_t inner_buf_ix = 0;
    for (size_t i = 0; i < serializer_writes.size(); ++i) {
        if (serializer_writes[i].buf_specified && serializer_writes[i].buf) {
            rassert(serializer_inner_bufs[inner_buf_ix]->block_id == serializer_writes[i].block_id);
            serializer_inner_bufs[inner_buf_ix++]->transaction_id = parent->cache->serializer->get_current_transaction_id(serializer_writes[i].block_id, serializer_writes[i].buf);
        }

        // TODO: The serializer's semantics of buf_specified is kinda weird in the case of deletions,
        // because deletions have buf_specified set to true, but buf being NULL.
        if (serializer_writes[i].buf_specified && !serializer_writes[i].buf) {
            // It's a deletion

            // We assume that all deleted blocks are reflected in the serializer's LBA (in case of the log serializer) by now
            // and will not get offered as read-ahead blocks anymore.
            // Therefore we can remove them from our reject_read_ahead_blocks list.
            // TODO: I don't like this implicit assumption (which is not really part of the tid_callback semantics). Change it.
            parent->reject_read_ahead_blocks.erase(serializer_writes[i].block_id);

            // Also we are now allowed to reuse the block id without further conflicts
            parent->cache->free_list.release_block_id(serializer_writes[i].block_id);
        }
    }
    transaction_ids_have_been_updated = true;

    // Allow new concurrent flushes to start from now on
    // (we had to wait until the transaction got passed to the serializer)
    parent->writeback_in_progress = false;

    // Also start the next sync now in case it was requested
    if (parent->start_next_sync_immediately) {
        parent->start_next_sync_immediately = false;
        parent->sync(NULL);
    }
}

void writeback_t::concurrent_flush_t::on_serializer_write_tid() {
    do_on_thread(parent->cache->home_thread(), boost::bind(&writeback_t::concurrent_flush_t::update_transaction_ids, this));
}

void writeback_t::concurrent_flush_t::on_serializer_write_txn() {
    do_on_thread(parent->cache->home_thread(), boost::bind(&writeback_t::concurrent_flush_t::do_cleanup, this));
}

bool writeback_t::concurrent_flush_t::do_cleanup() {
    parent->cache->assert_thread();
    rassert(transaction_ids_have_been_updated);
    
    /* We are done writing all of the buffers */

    // At this point it's definitely safe to release all the buffers
    for (size_t i = 0; i < buf_writers.size(); ++i) {
        delete buf_writers[i];
    }

    delete transaction;

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

