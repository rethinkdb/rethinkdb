#include "writeback.hpp"

template<class mc_config_t>
writeback_tmpl_t<mc_config_t>::writeback_tmpl_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold)
    : wait_for_flush(wait_for_flush),
      flush_timer_ms(flush_timer_ms),
      flush_threshold(flush_threshold),
      flush_timer(NULL),
      state(state_unstarted),
      cache(cache),
      start_next_sync_immediately(false),
      transaction(NULL) {
}

template<class mc_config_t>
writeback_tmpl_t<mc_config_t>::~writeback_tmpl_t() {
    assert(!flush_timer);
    gdelete(flush_lock);
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::start() {
    assert(state == state_unstarted);
    flush_lock = gnew<rwi_lock_t>();
    state = state_ready;
}

template<class mc_config_t>
bool writeback_tmpl_t<mc_config_t>::sync(sync_callback_t *callback) {

    if (state == state_ready) {
        /* Start the writeback process immediately */
        if (next_writeback_step()) {
            return true;
        } else {
            /* Wait until here before putting the callback into the callbacks list, because we
            don't want to call the callback if the writeback completes immediately. */
            if (callback) {
                switch(state) {
                    case state_locking:
                        sync_callbacks.push_back(callback);
                        break;
                    case state_write_bufs:
                        current_sync_callbacks.push_back(callback);
                        break;
                    default:
                        // We can't be in state_ready, because we just started writeback and it
                        // didn't complete. We can't be in state_locked or state_cleanup, because
                        // those states aren't really "states"; they're more like transitions.
                        fail("Writeback is in unexpected state");
                }
            }
            return false;
        }
    
    } else {
        /* There is a writeback currently in progress, but sync() has been called, so there is
        more data that needs to be flushed that didn't become part of the current sync. So we
        start another sync right after this one. */
        // TODO: If state == state_locking, we could probably still join the current writeback 
        // rather than waiting for the next one.
        start_next_sync_immediately = true;
        if (callback) sync_callbacks.push_back(callback);
        return false;
    }
}

template<class mc_config_t>
bool writeback_tmpl_t<mc_config_t>::sync_patiently(sync_callback_t *callback) {

    if (num_dirty_blocks() > 0) {
        if (callback) sync_callbacks.push_back(callback);
        return false;
        
    } else {
        // There's nothing to flush, so the sync is "done"
        return true;
    }
}

template<class mc_config_t>
bool writeback_tmpl_t<mc_config_t>::begin_transaction(transaction_t *txn,
        transaction_begin_callback_t *callback) {
    
    switch(txn->get_access()) {
        case rwi_read:
            return true;
        case rwi_write:
            // Lock the flush lock "for reading", but what we really mean is to lock it non-
            // exclusively because more than one write transaction can proceed at once.
            if (flush_lock->lock(rwi_read, txn)) {
                return true;
            } else {
                txn->begin_callback = callback;
                return false;
            }
        default:
            fail("Transaction access invalid.");
    }
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::on_transaction_commit(transaction_t *txn) {
    
    if (txn->get_access() == rwi_write) {
        flush_lock->unlock();
        
        /* At the end of every write transaction, check if the number of dirty blocks exceeds the
        threshold to force writeback to start. */
        if (num_dirty_blocks() > flush_threshold) {
            sync(NULL);
        } else if (num_dirty_blocks() > 0 && flush_timer_ms == 0) {
            sync(NULL);
        } else if (!flush_timer && flush_timer_ms != NEVER_FLUSH && num_dirty_blocks() > 0) {
            /* Start the flush timer so that the modified data doesn't sit in memory for too long
            without being written to disk. */
            flush_timer = get_cpu_context()->event_queue->
                fire_timer_once(flush_timer_ms, flush_timer_callback, this);
        }
    }
}

template<class mc_config_t>
unsigned int writeback_tmpl_t<mc_config_t>::num_dirty_blocks() {
    return dirty_bufs.size();
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::local_buf_t::set_dirty(bool _dirty) {
    if(!dirty && _dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        gbuf->cache->writeback.dirty_bufs.push_back(this);
    }
    if(dirty && !_dirty) {
        // We need to "unmark" the buf
        dirty = false;
        gbuf->cache->writeback.dirty_bufs.remove(this);
    }
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::flush_timer_callback(void *ctx) {
    writeback_tmpl_t *self = static_cast<writeback_tmpl_t *>(ctx);
    self->flush_timer = NULL;
    
    self->sync(NULL);
}

template<class mc_config_t>
bool writeback_tmpl_t<mc_config_t>::next_writeback_step() {

    if (state == state_ready) {
        
        // Cancel the flush timer because we're doing writeback now, so we don't need it to remind
        // us later. This happens only if the flush timer is running, and writeback starts for some
        // other reason before the flush timer goes off; if this writeback had been started by the
        // flush timer, then flush_timer would be NULL here, because flush_timer_callback sets it
        // to NULL.
        if (flush_timer) {
            get_cpu_context()->event_queue->cancel_timer(flush_timer);
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
        if (flush_lock->lock(rwi_write, this)) {
            state = state_locked;
        } else {
            state = state_locking;
            return false;
        }
    }
    
    if (state == state_locked) {
    
        assert(current_sync_callbacks.empty());

        current_sync_callbacks.append_and_clear(&sync_callbacks);

        /* Request read locks on all of the blocks we need to flush. */
        int num_writes = dirty_bufs.size();
        // TODO: optimize away dynamic allocation
        typename mc_config_t::serializer_t::write_t writes[num_writes];
        int i;
        typename intrusive_list_t<local_buf_t>::iterator it;
        for (it = dirty_bufs.begin(), i = 0; it != dirty_bufs.end(); i++) {
            buf_t *_buf = (*it).gbuf;
            it++;   // Increment the iterator so we can safely delete the thing it points to later
            
            bool do_delete = _buf->do_delete;
            
            // Acquire the blocks
            _buf->do_delete = false; /* Backdoor around acquire()'s assertion */
            buf_t *buf = transaction->acquire(_buf->get_block_id(), rwi_read, NULL);
            assert(buf);         // Acquire must succeed since we hold the flush_lock.
            assert(buf == _buf); // Acquire should return the same buf we stored earlier.
            
            // Fill the serializer structure
            if (!do_delete) {
                writes[i].block_id = buf->get_block_id();
                writes[i].buf = buf->data;
                writes[i].callback = buf;
#ifndef NDEBUG
                buf->active_callback_count ++;
#endif

            } else {
            
                writes[i].block_id = buf->get_block_id();
                writes[i].buf = NULL;   // NULL indicates a deletion
                writes[i].callback = NULL;
                
                // Dodge assertions so we can delete the buf
                buf->writeback_buf.dirty = false;
                buf->release();
                dirty_bufs.remove(&buf->writeback_buf);
                
                delete buf;
            }
        }
        dirty_bufs.clear();
        
        flush_lock->unlock(); // Write transactions can now proceed again.
        
        /* Start writing all the dirty bufs down, as a transaction. */
        
        // TODO(NNW): Now that the serializer/aio-system breaks writes up into
        // chunks, we may want to worry about submitting more heavily contended
        // bufs earlier in the process so more write FSMs can proceed sooner.
        
        if (num_writes == 0 || cache->serializer.do_write(writes, num_writes, this)) {
            state = state_cleanup;
        } else {
            state = state_write_bufs;
            return false;
        }
    }
    
    if (state == state_cleanup) {
    
        /* We are done writing all of the buffers */
        
        bool committed __attribute__((unused)) = transaction->commit(NULL);
        assert(committed); // Read-only transactions commit immediately.
        transaction = NULL;
                    
        while (!current_sync_callbacks.empty()) {
            sync_callback_t *cb = current_sync_callbacks.head();
            current_sync_callbacks.remove(cb);
            cb->on_sync();
        }
        
        // Don't set state to state_ready until after we call all the sync callbacks, because
        // otherwise it would crash if a sync callback called sync().
        state = state_ready;

        if (start_next_sync_immediately) {
            start_next_sync_immediately = false;
            sync(NULL);
        }
        
        return true;
    }
    
    fail("Invalid state.");
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::buf_was_written(buf_t *buf) {
    assert(buf);
    buf->writeback_buf.dirty = false;
    buf->release();
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::on_lock_available() {
    assert(state == state_locking);
    state = state_locked;
    next_writeback_step();
}

template<class mc_config_t>
void writeback_tmpl_t<mc_config_t>::on_serializer_write_txn() {
    assert(state == state_write_bufs);
    state = state_cleanup;
    next_writeback_step();
}
