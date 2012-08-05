#include "buffer_cache/mirrored/writeback.hpp"

#include <math.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/serializer.hpp"

// TODO: We added a writeback->possibly_unthrottle_transactions() call
// in the begin_transaction_fsm_t(..) constructor, where did that get
// merged to now?

writeback_t::writeback_t(
        cache_t *_cache,
        bool _wait_for_flush,
        unsigned int _flush_timer_ms,
        unsigned int _flush_threshold,
        unsigned int _max_dirty_blocks,
        unsigned int _flush_waiting_threshold,
        unsigned int _max_concurrent_flushes
        ) :
    wait_for_flush(_wait_for_flush),
    flush_waiting_threshold(_flush_waiting_threshold),
    max_concurrent_flushes(_max_concurrent_flushes),
    max_dirty_blocks(_max_dirty_blocks),
    flush_time_randomizer(_flush_timer_ms),
    flush_threshold(_flush_threshold),
    flush_timer(NULL),
    writeback_in_progress(false),
    active_flushes(0),
    dirty_block_semaphore(_max_dirty_blocks),
    force_patch_storage_flush(false),
    cache(_cache),
    start_next_sync_immediately(false),
    to_pulse_when_last_active_flush_finishes(NULL) {

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
        start_concurrent_flush();
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
    inner_buf_t *gbuf = static_cast<inner_buf_t *>(this);
    if (!dirty && _dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.push_back(this);
            /* Use `force_lock()` to prevent deadlocks; `co_lock()` could block. */
            gbuf->cache->writeback.dirty_block_semaphore.force_lock();
        }
        ++gbuf->cache->stats->pm_n_blocks_dirty;
    }
    if (dirty && !_dirty) {
        // We need to "unmark" the buf
        dirty = false;
        if (!recency_dirty) {
            gbuf->cache->writeback.dirty_bufs.remove(this);
            gbuf->cache->writeback.dirty_block_semaphore.unlock();
        }
        --gbuf->cache->stats->pm_n_blocks_dirty;
    }
}

void writeback_t::local_buf_t::set_recency_dirty(bool _recency_dirty) {
    inner_buf_t *gbuf = static_cast<inner_buf_t *>(this);
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
    inner_buf_t *gbuf = static_cast<inner_buf_t *>(this);
    gbuf->cache->writeback.deleted_blocks.push_back(gbuf->block_id);

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

    self->cache->stats->pm_patches_size_ratio.record(self->cache->get_max_patches_size_ratio());

    /*
     * Update the max_patches_size_ratio. If we detect that the previous writeback
     * is still active and the max_dirty_blocks_limit is exhausted at a level of
     * at least RAISE_PATCHES_RATIO_AT_FRACTION_OF_UNSAVED_DATA_LIMIT, we
     * consider the system to be i/o bound. In this case, we proactively decrease
     * the max_patches_size_ratio towards MAX_PATCHES_SIZE_RATIO_MIN, to increase
     * the usage of patches and reduce the amount of i/o.
     * Otherwise, we consider the system to be bound by something other than i/o
     * and gradually increase max_patches_size_ratio towards MAX_PATCHES_SIZE_RATIO_MAX
     * to save the overhead associated with managing and writing patches.
     */
    if (self->active_flushes < self->max_concurrent_flushes || self->num_dirty_blocks() < self->max_dirty_blocks * RAISE_PATCHES_RATIO_AT_FRACTION_OF_UNSAVED_DATA_LIMIT) {
        /* The currently running writeback probably finished on-time. (of we have enough headroom left before hitting the unsaved data limit)
        Adjust max_patches_size_ratio to trade i/o efficiency for CPU cycles */
        if (!self->wait_for_flush) {
            self->cache->adjust_max_patches_size_ratio_toward_minimum();
        }
    } else {
        /* The currently running writeback apparently takes too long.
        try to reduce that bottleneck by adjusting max_patches_size_ratio */
        if (!self->wait_for_flush) {
            self->cache->adjust_max_patches_size_ratio_toward_maximum();
        }
    }

    /* Don't sync if we're in the shutdown process, because if we do that we'll trip an rassert() on
    the cache, and besides we're about to sync anyway. */
    if (!self->cache->shutting_down && (self->num_dirty_blocks() > 0 || self->sync_callbacks.size() > 0)) {
        self->sync(NULL);
    }
}

class writeback_t::buf_writer_t :
    public iocallback_t,
    public thread_message_t,
    public home_thread_mixin_t {
    cond_t self_cond_;

public:
    struct launch_callback_t :
        public serializer_write_launched_callback_t,
        public thread_message_t,
        public home_thread_mixin_t {
        writeback_t *parent;
        intrusive_ptr_t<standard_block_token_t> token;

    private:
        friend class buf_writer_t;
        cond_t finished_;

        scoped_ptr_t<mc_buf_lock_t> buf;

    public:
        void on_write_launched(const intrusive_ptr_t<standard_block_token_t>& tok) {
            token = tok;
            if (continue_on_thread(home_thread(), this)) on_thread_switch();
        }
        void on_thread_switch() {
            assert_thread();
            // update buf's (or one of it's snapshot's) data token appropriately
            buf->inner_buf->update_data_token(buf->get_data_read(), token);
            token.reset();
            // Update block sequence id.
            buf->inner_buf->block_sequence_id = parent->cache->serializer->get_block_sequence_id(
                buf->get_block_id(), buf->get_data_read());
            // We're done here.
            finished_.pulse();
        }
        void wait_until_sequence_ids_updated() {
            finished_.wait();
        }
    } launch_cb;

    buf_writer_t(writeback_t *wb, mc_buf_lock_t *buf) {
        launch_cb.parent = wb;
        launch_cb.buf.init(buf);
        launch_cb.parent->cache->assert_thread();
        /* When we spawn a flush, the block ceases to be dirty, so we release the
        semaphore. To avoid releasing a tidal wave of write transactions every time
        the flush starts, we have the writer acquire the semaphore and release it only
        once the block is safely on disk. */
        launch_cb.parent->dirty_block_semaphore.force_lock(1);
    }
    void on_io_complete() {
        if (continue_on_thread(home_thread(), this)) on_thread_switch();
    }
    void on_thread_switch() {
        assert_thread();
        launch_cb.parent->dirty_block_semaphore.unlock();
        // Ideally, if we were done updating the block sequence id, we would be able to release the
        // buffer now. However, we're not in coroutine context, and releasing a buffer could require
        // releasing a snapshot, which could cause an ls_block_token to hit refcount 0 and be
        // destroyed, which requires being in coroutine context to switch back to the serializer
        // thread and unregister it. So, we cannot do that here.
        // TODO: fix this^ somehow.
        self_cond_.pulse();
    }
    void wait_for_finish() {
        self_cond_.wait();
    }
    ~buf_writer_t() {
        assert_thread();
        rassert(self_cond_.is_pulsed());
        rassert(launch_cb.finished_.is_pulsed());
    }
};

struct writeback_t::flush_state_t {
    bool block_sequence_ids_have_been_updated;
    std::vector<buf_writer_t *> buf_writers;
    // Writes to submit to the serializer
    std::vector<serializer_write_t> serializer_writes;
    flush_state_t() : block_sequence_ids_have_been_updated(false) {}
};

void writeback_t::start_concurrent_flush() {
    if (dirty_bufs.size() == 0 && sync_callbacks.size() == 0)
        // No flush necessary
        return;

    rassert(!writeback_in_progress);
    writeback_in_progress = true;
    ++active_flushes;

    coro_t::spawn(boost::bind(&writeback_t::do_concurrent_flush, this));
}

// TODO(rntz) break this up into smaller functions
void writeback_t::do_concurrent_flush() {
    ticks_t start_time;
    cache->stats->pm_flushes_locking.begin(&start_time);
    cache->assert_thread();

    // As we cannot afford waiting for blocks to get loaded from disk while holding the flush lock,
    // we instead reclaim some space in the on-disk patch storage now.
    ticks_t start_time2;
    cache->stats->pm_flushes_diff_flush.begin(&start_time2);
    // TODO: What are these magic constants doing here?
    unsigned int blocks_to_flush = uint64_t(dirty_bufs.size()) * 100LL / cache->get_block_size().value() + 1;
    if (force_patch_storage_flush) {
        blocks_to_flush = std::max(cache->patch_disk_storage->get_number_of_log_blocks() / 20 + 1, blocks_to_flush);
        force_patch_storage_flush = false;
    }
    cache->patch_disk_storage->clear_n_oldest_blocks(blocks_to_flush);
    cache->stats->pm_flushes_diff_flush.end(&start_time2);

    /* Start a read transaction so we can request bufs. */
    transaction_t *transaction;
    {
        // This was originally for some hack where we change the value
        // of shutting_down, but I don't care to remove it.
        ASSERT_NO_CORO_WAITING;

        // It's a read transaction, that's why we use repli_timestamp_t::invalid.
        i_am_writeback_t iam;
        transaction = new mc_transaction_t(cache, rwi_read, iam);
    }

    flush_state_t state;
    intrusive_list_t<sync_callback_t> current_sync_callbacks; // Callbacks for this sync

    // Acquire flush lock to force write txn completion, and perform necessary preparations
    {
        /* Acquire exclusive flush_lock, forcing all write txns to complete. */
        rwi_lock_t::write_acq_t flush_lock_acq(&flush_lock);
        rassert(writeback_in_progress);

        cache->stats->pm_flushes_locking.end(&start_time);

        // Move callbacks to locals only after we got the lock.
        // That way callbacks coming in while waiting for the flush lock
        // can still go into this flush.
        current_sync_callbacks.append_and_clear(&sync_callbacks);

        // Also, at this point we can still clear the start_next_sync_immediately...
        start_next_sync_immediately = false;

        // Go through the different flushing steps...
        flush_prepare_patches();
        cache->stats->pm_flushes_writing.begin(&start_time);
        flush_acquire_bufs(transaction, &state);
    }

    // Now that preparations are complete, send the writes to the serializer
    if (!state.serializer_writes.empty()) {
        on_thread_t switcher(cache->serializer->home_thread());
        do_writes(cache->serializer, state.serializer_writes, cache->writes_io_account.get());
    }

    // Once transaction has completed, perform cleanup.
    for (size_t i = 0; i < state.serializer_writes.size(); ++i) {
        const serializer_write_t &write = state.serializer_writes[i];
        if (write.action_type != serializer_write_t::DELETE) {
            break;
        }

        // All deleted blocks are now reflected in the serializer's LBA and will not get offered as
        // read-ahead blocks anymore. Therefore we can remove them from our reject_read_ahead_blocks
        // list.
        reject_read_ahead_blocks.erase(write.block_id);

        // Also we are now allowed to reuse the block id without further conflicts
        cache->free_list.release_block_id(write.block_id);
    }
    state.serializer_writes.clear();

    // Wait for block sequence ids to be updated.
    for (size_t i = 0; i < state.buf_writers.size(); ++i)
        state.buf_writers[i]->launch_cb.wait_until_sequence_ids_updated();

    // Allow new concurrent flushes to start from now on
    writeback_in_progress = false;

    // Also start the next sync now in case it was requested
    if (start_next_sync_immediately) {
        start_next_sync_immediately = false;
        sync(NULL);
    }

    // Wait for the buf_writers to finish running their io callbacks
    for (size_t i = 0; i < state.buf_writers.size(); ++i) {
        state.buf_writers[i]->wait_for_finish();
        delete state.buf_writers[i];
    }
    state.buf_writers.clear();
    delete transaction;

    while (!current_sync_callbacks.empty()) {
        sync_callback_t *cb = current_sync_callbacks.head();
        current_sync_callbacks.remove(cb);
        cb->on_sync();
    }

    cache->stats->pm_flushes_writing.end(&start_time);
    --active_flushes;

    // Try again to start the next sync now.  If we didn't do this,
    // then the following may occur.  If active_flushes ==
    // max_active_flushes and all the flush operations are waiting for
    // I/O to complete (they've all passed the previous attempt to
    // check start_next_sync_immediately), and if we have no active
    // transactions (because our active transaction is blocked trying
    // to acquire the dirty block semaphore) then
    // start_next_sync_immediately will be true and we'll finish all
    // our flushes without starting a new one, and the transaction
    // waiting on the dirty block semaphore will never get to go,
    // which means the flush timer will never be reactivated.  So
    // we'll have a deadlock. See issue #457.
    if (start_next_sync_immediately) {
        start_next_sync_immediately = false;
        sync(NULL);
    }

    if (active_flushes == 0) {
        if (to_pulse_when_last_active_flush_finishes) {
            to_pulse_when_last_active_flush_finishes->pulse();
        }
    }
}

void writeback_t::flush_prepare_patches() {
    rassert(writeback_in_progress);

    /* Write patches for blocks we don't want to flush now */
    // Please note: Writing patches to the oocore_storage can still alter the dirty_bufs list!
    // (because some of the patch storage blocks will become dirty)
    // The iteration below doesn't have a problem with that, but be careful if you
    // want to change it.
    ticks_t start_time2;
    cache->stats->pm_flushes_diff_store.begin(&start_time2);
    bool patch_storage_failure = false;
    unsigned int patches_stored = 0;
    for (local_buf_t *lbuf = dirty_bufs.head(); lbuf; lbuf = dirty_bufs.next(lbuf)) {
        inner_buf_t *inner_buf = static_cast<inner_buf_t *>(lbuf);

        // If the patch storage failed before (which usually happens if it ran out of space),
        // we don't even try to write patches for this buffer. Instead we set needs_flush,
        // causing the block itself to be written to disk instead of its patches.
        if (patch_storage_failure && lbuf->get_dirty() && !lbuf->needs_flush()) {
            lbuf->set_needs_flush(true);
            cache->patch_memory_storage.drop_patches(inner_buf->block_id);
        }

        if (!lbuf->needs_flush() && lbuf->get_dirty() && inner_buf->next_patch_counter > 1) {
            const block_sequence_id_t block_sequence_id = inner_buf->block_sequence_id;
            if (cache->patch_memory_storage.has_patches_for_block(inner_buf->block_id)) {
#ifndef NDEBUG
                patch_counter_t previous_patch_counter = 0;
#endif
                std::pair<patch_memory_storage_t::const_patch_iterator, patch_memory_storage_t::const_patch_iterator>
                    range = cache->patch_memory_storage.patches_for_block(inner_buf->block_id);

                while (range.second != range.first) {
                    // Why do we write patches in reverse oder?
                    // Because that way we first get the most recent patches,
                    // and can then just break out of the loop as soon as we hit
                    // the first patch that is older than last_patch_materialized.
                    --range.second;

                    rassert(block_sequence_id > NULL_BLOCK_SEQUENCE_ID);
                    rassert(previous_patch_counter == 0 || (*range.second)->get_patch_counter() == previous_patch_counter - 1);
                    // Write only those patches that we have not written in a previous
                    // writeback. Use lbuf->last_patch_materialized to make
                    // that decision. This way we never have duplicate patches on disk.
                    if (lbuf->last_patch_materialized() < (*range.second)->get_patch_counter()) {
                        if (!cache->patch_disk_storage->store_patch(*range.second, block_sequence_id)) {
                            patch_storage_failure = true;
                            // We were not able to store all the patches to disk,
                            // so we fall back to re-writing the block itself.
                            lbuf->set_needs_flush(true);
                            // Dropping the patches invalidates our iterators in range,
                            // but we can simply break out of the loop at this point.
                            cache->patch_memory_storage.drop_patches(inner_buf->block_id);
                            break;
                        } else {
                            patches_stored++;
                        }
                    } else {
                        // We hit a patch that we had written to disk before.
                        // Because we are iterating over patches from most recent to older,
                        // we can just break out of the loop at this point.
                        break;
                    }
#ifndef NDEBUG
                    previous_patch_counter = (*range.second)->get_patch_counter();
#endif
                }

                if (!patch_storage_failure) {
                    lbuf->set_last_patch_materialized(cache->patch_memory_storage.last_patch_materialized_or_zero(inner_buf->block_id));
                }
                // (if there was a patch_storage_failure, lbuf->needs_flush got set
                // and we are going to reset last_patch_materialized a few lines later...)
            }
        }

        if (lbuf->needs_flush()) {
            // Because the block will be rewritten and therefore receive a new
            // block_sequence_id, we can reset the patch counter.
            // (Remember: The applicability of a patch is determined by the
            // block_sequence_id that it applies to, while the patch_counter
            // provides an ordering over all patches that apply to a certain
            // version of a block.)
            inner_buf->next_patch_counter = 1;
            lbuf->set_last_patch_materialized(0);
        }
    }
    cache->stats->pm_flushes_diff_store.end(&start_time2);
    if (patch_storage_failure)
        force_patch_storage_flush = true; // Make sure we resolve the storage space shortage before the next flush...

    cache->stats->pm_flushes_diff_patches_stored.record(patches_stored);
    if (patch_storage_failure)
        cache->stats->pm_flushes_diff_storage_failures.record(patches_stored);
}

void writeback_t::flush_acquire_bufs(transaction_t *transaction, flush_state_t *state) {
    /* Request read locks on all of the blocks we need to flush. */
    // Log the size of this flush
    cache->stats->pm_flushes_blocks.record(dirty_bufs.size());

    // Request read locks on all of the blocks we need to flush.
    state->serializer_writes.reserve(deleted_blocks.size() + dirty_bufs.size());

    // Write deleted block_ids.
    for (size_t i = 0; i < deleted_blocks.size(); i++) {
        state->serializer_writes.push_back(serializer_write_t::make_delete(deleted_blocks[i]));
    }
    deleted_blocks.clear();

    unsigned int really_dirty = 0;

    while (local_buf_t *lbuf = dirty_bufs.head()) {
        inner_buf_t *inner_buf = static_cast<inner_buf_t *>(lbuf);

        bool buf_needs_flush = lbuf->needs_flush();
        //bool buf_dirty = lbuf->dirty;
        bool recency_dirty = lbuf->get_recency_dirty();

        // Removes it from dirty_bufs
        lbuf->set_dirty(false);
        lbuf->set_recency_dirty(false);
        lbuf->set_needs_flush(false);

        if (buf_needs_flush) {
            ++really_dirty;

            rassert(!inner_buf->do_delete);

            // Acquire the blocks
            buf_lock_t *buf;
            {
                // Acquire always succeeds, but sometimes it blocks.
                // But it won't block because we hold the flush lock.
                ASSERT_NO_CORO_WAITING;
                buf = new buf_lock_t(transaction, inner_buf->block_id, rwi_read_outdated_ok);
            }

            // Fill the serializer structure
            buf_writer_t *buf_writer = new buf_writer_t(this, buf);
            state->buf_writers.push_back(buf_writer);
            state->serializer_writes.push_back(
                serializer_write_t::make_update(inner_buf->block_id,
                                                inner_buf->subtree_recency,
                                                buf->get_data_read(),
                                                buf_writer,
                                                &buf_writer->launch_cb));
        } else if (recency_dirty) {
            // No need to acquire the block, since we're only writing its recency & don't need its contents.
            state->serializer_writes.push_back(serializer_write_t::make_touch(inner_buf->block_id, inner_buf->subtree_recency));
        }

    }

    cache->stats->pm_flushes_blocks_dirty.record(really_dirty);
}
