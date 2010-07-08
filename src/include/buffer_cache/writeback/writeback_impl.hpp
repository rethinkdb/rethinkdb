
#ifndef __BUFFER_CACHE_WRITEBACK_IMPL_HPP__
#define __BUFFER_CACHE_WRITEBACK_IMPL_HPP__

template <class config_t>
writeback_tmpl_t<config_t>::writeback_tmpl_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_interval_ms,
        unsigned int force_flush_threshold)
    : safety_timer(NULL),
      wait_for_flush(wait_for_flush),
      interval_ms(flush_interval_ms),
      force_flush_threshold(force_flush_threshold),
      cache(cache),
      num_txns(0),
      shutdown_callback(NULL),
      final_sync(NULL),
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
    
    // If interval_ms is NEVER_FLUSH, then neither timer should be started
    if (interval_ms != NEVER_FLUSH) {
        // 50ms is an arbitrary interval for the threshold timer to run on
        get_cpu_context()->event_queue->add_timer(50, threshold_timer_callback, this);
    }
    start_safety_timer();
}

template <class config_t>
void writeback_tmpl_t<config_t>::shutdown(sync_callback_t *callback) {
    assert(shutdown_callback == NULL);
    shutdown_callback = callback;
    if (!num_txns && state == state_none) // If num_txns, commit() will do this
        sync(callback);
}

template <class config_t>
void writeback_tmpl_t<config_t>::sync(sync_callback_t *callback) {
    sync_callbacks.push_back(callback);
    // Start a new writeback process if one isn't in progress.
    if (state == state_none)
        writeback(NULL);
}

template <class config_t>
bool writeback_tmpl_t<config_t>::begin_transaction(transaction_t *txn) {
    assert(txn->get_access() == rwi_read || txn->get_access() == rwi_write);
    // TODO(NNW): If there's ever any asynchrony between socket reads and
    // begin_transaction, we'll need a better check here.
    assert(shutdown_callback == NULL || final_sync);
    num_txns++;
    if (txn->get_access() == rwi_read)
        return true;
    bool locked = flush_lock->lock(rwi_read, txn);
    return locked;
}

template <class config_t>
bool writeback_tmpl_t<config_t>::commit(transaction_t *txn,
        transaction_commit_callback_t *callback) {
    if (!--num_txns && shutdown_callback != NULL) {
        // All txns shut down, start final sync.
        sync(shutdown_callback);
    }
    if (txn->get_access() == rwi_read)
        return true;
    flush_lock->unlock();
    if (!wait_for_flush)
        return true;
    txns.push_back(new txn_state_t(txn, callback));
    return false;
}

template <class config_t>
void writeback_tmpl_t<config_t>::aio_complete(buf_t *buf, bool written) {
    if (written)
        writeback(buf);
}

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
void writeback_tmpl_t<config_t>::start_safety_timer() {
    if (interval_ms != NEVER_FLUSH && interval_ms != 0) {
        event_queue_t *eq = get_cpu_context()->event_queue;
        if (safety_timer) {
            eq->cancel_timer(safety_timer);
            safety_timer = NULL;
        }
        safety_timer = eq->fire_timer_once(interval_ms, safety_timer_callback, this);
    }
}

template <class config_t>
void writeback_tmpl_t<config_t>::safety_timer_callback(void *ctx) {
    writeback_tmpl_t *self = static_cast<writeback_tmpl_t *>(ctx);
    self->safety_timer = NULL;
    // TODO(NNW): We can't start writeback when it's already started, but we
    // may want a more thorough way of dealing with this case.
    if (self->state == state_none)
        self->writeback(NULL);
}

template <class config_t>
void writeback_tmpl_t<config_t>::threshold_timer_callback(void *ctx) {
    writeback_tmpl_t *self = static_cast<writeback_tmpl_t *>(ctx);
    if (self->state == state_none) {
        if (self->dirty_bufs.size() > self->force_flush_threshold) {
            self->writeback(NULL);
        }
    }
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
        
        /* Start a read transaction so we can request bufs. */
        assert(transaction == NULL);
        if (shutdown_callback) // Backdoor around "no new transactions" assert.
            final_sync = true;
        transaction = cache->begin_transaction(rwi_read, NULL);
        final_sync = false;
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
        assert(flush_txns.empty());

        flush_txns.append_and_clear(&txns);

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
            /* Notify all waiting transactions of completion. */
            typename intrusive_list_t<txn_state_t>::iterator it;
            for (it = flush_txns.begin(); it != flush_txns.end(); ) {
                txn_state_t *txn_state = &*it;
                it++;
                txn_state->txn->committed(txn_state->callback);
                flush_txns.remove(txn_state);
                delete txn_state;
            }
            assert(flush_txns.empty());

            for (typename std::vector<sync_callback_t*, gnew_alloc<sync_callback_t*> >::iterator it =
                     sync_callbacks.begin(); it != sync_callbacks.end(); ++it)
                (*it)->on_sync();
            sync_callbacks.clear();

            /* Reset all of our state. */
            bool committed = transaction->commit(NULL);
            assert(committed); // Read-only transactions commit immediately.
            transaction = NULL;
            state = state_none;
            
            // The timer for the next flush is not started until the current flush is complete. This
            // is a bit dangerous because if anything causes the current flush to abort prematurely,
            // the flush timer will never get restarted. As of 2010-06-28 this should never happen,
            // but keep it in mind when changing the behavior of the writeback.
            start_safety_timer();
            
            //printf("Writeback complete\n");
        } else {
            //printf("Flush bufs, waiting for %ld more\n", flush_bufs.size());
        }
    }
}

#endif  // __BUFFER_CACHE_WRITEBACK_IMPL_HPP__
