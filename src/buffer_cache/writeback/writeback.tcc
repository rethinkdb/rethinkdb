
#ifndef __BUFFER_CACHE_WRITEBACK_TCC__
#define __BUFFER_CACHE_WRITEBACK_TCC__

template <class config_t>
writeback_tmpl_t<config_t>::writeback_tmpl_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold)
    : flush_timer(NULL),
      wait_for_flush(wait_for_flush),
      flush_timer_ms(flush_timer_ms),
      flush_threshold(flush_threshold),
      cache(cache),
      num_txns(0),
      start_next_sync_immediately(false),
      shutdown_callback(NULL),
      in_shutdown_sync(false),
      transaction_backdoor(false),
      state(state_none),
      transaction(NULL) {
}

template <class config_t>
writeback_tmpl_t<config_t>::~writeback_tmpl_t() {
    gdelete(flush_lock);
}

template <class config_t>
void writeback_tmpl_t<config_t>::start() {
    flush_lock =
        gnew<rwi_lock_t>(&get_cpu_context()->event_queue->message_hub,
                         get_cpu_context()->event_queue->queue_id);
}

template <class config_t>
void writeback_tmpl_t<config_t>::shutdown(sync_callback_t *callback) {
    assert(shutdown_callback == NULL);
    shutdown_callback = callback;
    if (!num_txns) // If num_txns, commit() will do this
        sync(callback);
}

template <class config_t>
void writeback_tmpl_t<config_t>::sync(sync_callback_t *callback) {

    if (callback) sync_callbacks.push_back(callback);
    
    // TODO: If state == state_locking, we could probably still join the current writeback rather
    // than waiting for the next one.
    
    if (state == state_none) {
        /* Start the writeback process immediately */
        writeback(NULL);
    } else {
        /* There is a writeback currently in progress, but sync() has been called, so there is more
        data that needs to be flushed that didn't become part of the current sync. So we start
        another sync right after this one. */
        start_next_sync_immediately = true;
    }
}

template <class config_t>
bool writeback_tmpl_t<config_t>::begin_transaction(transaction_t *txn) {
   
    assert(txn->get_access() == rwi_read || txn->get_access() == rwi_write);
    
    // TODO(NNW): If there's ever any asynchrony between socket reads and
    // begin_transaction, we'll need a better check here.
    assert(shutdown_callback == NULL || transaction_backdoor);
    
    num_txns++;
    
    if (txn->get_access() == rwi_read) return true;
    bool locked = flush_lock->lock(rwi_read, txn);
    return locked;
}

template <class config_t>
bool writeback_tmpl_t<config_t>::commit(transaction_t *txn) {
    
    num_txns --;
    
    if (txn->get_access() == rwi_write) {
        flush_lock -> unlock();
    }
    
    if (num_txns == 0 && shutdown_callback != NULL && !in_shutdown_sync) {
        // All txns shut down, start final sync.
        
        // So we don't do this again when the final sync's transaction commits
        in_shutdown_sync = true;
        
        sync(shutdown_callback);
    }
    
    if (txn->get_access() == rwi_write) {
        /* At the end of every write transaction, check if the number of dirty blocks exceeds the
        threshold to force writeback to start. */
        if (num_dirty_blocks() > flush_threshold) {
            sync(NULL);
        }
        /* Otherwise, start the flush timer so that the modified data doesn't sit in memory for too
        long without being written to disk. */
        else if (!flush_timer && flush_timer_ms != NEVER_FLUSH) {
            flush_timer = get_cpu_context()->event_queue->
                fire_timer_once(flush_timer_ms, flush_timer_callback, this);
        }
    }
    
    if (txn->get_access() == rwi_write && wait_for_flush) {
        /* Push the callback in manually rather than calling sync(); we will patiently wait for the
        next sync to come naturally. */
        sync_callbacks.push_back(txn);
        return false;
    } else {
        return true;
    }
}

template <class config_t>
void writeback_tmpl_t<config_t>::aio_complete(buf_t *buf, bool written) {
    if (written)
        writeback(buf);
}

template <class config_t>
unsigned int writeback_tmpl_t<config_t>::num_dirty_blocks() {
    return dirty_bufs.size();
}

#ifndef NDEBUG
template <class config_t>
void writeback_tmpl_t<config_t>::deadlock_debug() {
    printf("\n----- Writeback -----\n");
    const char *st_name;
    switch(state) {
        case state_none: st_name = "state_none"; break;
        case state_locking: st_name = "state_locking"; break;
        case state_locked: st_name = "state_locked"; break;
        case state_write_bufs: st_name = "state_write_bufs"; break;
        default: st_name = "<invalid state>"; break;
    }
    printf("state = %s\n", st_name);
}
#endif

template <class config_t>
void writeback_tmpl_t<config_t>::local_buf_t::set_dirty(buf_t *super) {
    // 'super' is actually 'this', but as a buf_t* instead of a local_buf_t*
    if(!dirty) {
        // Mark block as dirty if it hasn't been already
        dirty = true;
        writeback->dirty_bufs.push_back(super);
    }
}

template <class config_t>
void writeback_tmpl_t<config_t>::flush_timer_callback(void *ctx) {
    writeback_tmpl_t *self = static_cast<writeback_tmpl_t *>(ctx);
    self->flush_timer = NULL;
    
    self->sync(NULL);
}

template <class config_t>
void writeback_tmpl_t<config_t>::on_lock_available() {
    assert(state == state_locking);
    if (state == state_locking) {
        state = state_locked;
        writeback(NULL);
    }
}

template <class config_t>
void writeback_tmpl_t<config_t>::writeback(buf_t *buf) {
    //printf("Writeback being called, state %d\n", state);

    if (state == state_none) {
        
        assert(buf == NULL);
        
        // Cancel the flush timer because we're doing writeback now, so we don't need it to remind
        // us later
        if (flush_timer) {
            get_cpu_context()->event_queue->cancel_timer(flush_timer);
            flush_timer = NULL;
        }
        
        /* Start a read transaction so we can request bufs. */
        assert(transaction == NULL);
        if (shutdown_callback) // Backdoor around "no new transactions" assert.
            transaction_backdoor = true;
        transaction = cache->begin_transaction(rwi_read, NULL);
        transaction_backdoor = false;
        assert(transaction != NULL); // Read txns always start immediately.

        /* Request exclusive flush_lock, forcing all write txns to complete. */
        state = state_locking;
        bool locked = flush_lock->lock(rwi_write, this);
        if (locked) {
            state = state_locked;
        }
    }
    if (state == state_locked) {
        assert(buf == NULL);
        assert(flush_bufs.empty());
        assert(current_sync_callbacks.empty());

        current_sync_callbacks.append_and_clear(&sync_callbacks);

        /* Request read locks on all of the blocks we need to flush. */
        // TODO: optimize away dynamic allocation
        typename serializer_t::write *writes =
            (typename serializer_t::write*)calloc(dirty_bufs.size(), sizeof *writes);
        int i;
        typename intrusive_list_t<buf_t>::iterator it;
        for (it = dirty_bufs.begin(), i = 0; it != dirty_bufs.end(); it++, i++) {
            buf_t *_buf = &*it;

            // Acquire the blocks
            buf_t *buf = transaction->acquire(_buf->get_block_id(), rwi_read, NULL);
            assert(buf);         // Acquire must succeed since we hold the flush_lock.
            assert(buf == _buf); // Acquire should return the same buf we stored earlier.

            // Fill the serializer structure
            writes[i].block_id = buf->get_block_id();
            writes[i].buf = buf->ptr();
            writes[i].callback = buf;
            
#ifndef NDEBUG
            buf->active_callback_count ++;
#endif
        }
        flush_bufs.append_and_clear(&dirty_bufs);
        
        flush_lock->unlock(); // Write transactions can now proceed again.

        /* Start writing all the dirty bufs down, as a transaction. */
        // TODO(NNW): Now that the serializer/aio-system breaks writes up into
        // chunks, we may want to worry about submitting more heavily contended
        // bufs earlier in the process so more write FSMs can proceed sooner.
        if (flush_bufs.size())
            cache->do_write(get_cpu_context()->event_queue, writes,
                            flush_bufs.size());
        free(writes);
        state = state_write_bufs;
    }
    if (state == state_write_bufs) {
        if (buf) {
            flush_bufs.remove(buf);
            buf->set_clean();
            buf->release();
        }
        if (flush_bufs.empty()) {
            /* We are done writing all of the buffers */
            
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed); // Read-only transactions commit immediately.
            transaction = NULL;
                        
            while (!current_sync_callbacks.empty()) {
                sync_callback_t *cb = current_sync_callbacks.head();
                current_sync_callbacks.remove(cb);
                cb->on_sync();
            }

            state = state_none;

            if (start_next_sync_immediately) {
                start_next_sync_immediately = false;
                writeback(NULL);
            }
        }
    }
}

#endif  // __BUFFER_CACHE_WRITEBACK_TCC__
