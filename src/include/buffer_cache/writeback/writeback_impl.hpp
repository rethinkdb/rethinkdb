
#ifndef __BUFFER_CACHE_WRITEBACK_IMPL_HPP__
#define __BUFFER_CACHE_WRITEBACK_IMPL_HPP__

template <class config_t>
writeback_tmpl_t<config_t>::writeback_tmpl_t(cache_t *cache,
        bool wait_for_flush, unsigned int flush_interval_ms)
    : wait_for_flush(wait_for_flush),
      interval_ms(flush_interval_ms),
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
    start_flush_timer();
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
void writeback_tmpl_t<config_t>::local_buf_t::set_dirty(buf_t *super) {
    // 'super' is actually 'this', but as a buf_t* instead of a local_buf_t*
    dirty = true;
    writeback->dirty_blocks.insert(super);
}

template <class config_t>
void writeback_tmpl_t<config_t>::start_flush_timer() {
    if (interval_ms != NEVER_FLUSH && interval_ms != 0) {
        get_cpu_context()->event_queue->fire_timer_once(interval_ms, timer_callback, this);
    }
}

template <class config_t>
void writeback_tmpl_t<config_t>::timer_callback(void *ctx) {
    // TODO(NNW): We can't start writeback when it's already started, but we
    // may want a more thorough way of dealing with this case.
    if (static_cast<writeback_tmpl_t *>(ctx)->state == state_none)
        static_cast<writeback_tmpl_t *>(ctx)->writeback(NULL);
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
        if (locked)
            state = state_locked;
    }
    if (state == state_locked) {
        assert(buf == NULL);
        assert(flush_bufs.empty());
        assert(flush_txns.empty());

        flush_txns.append_and_clear(&txns);

        /* Request read locks on all of the blocks we need to flush. */
        for (typename std::set<buf_t*>::iterator it = dirty_blocks.begin();
             it != dirty_blocks.end(); ++it) {
            buf_t *buf = transaction->acquire((*it)->get_block_id(), rwi_read, NULL);
            assert(buf);        // Acquire must succeed since we hold the flush_lock.
            assert(buf == *it); // Acquire should return the same buf we stored earlier.
            flush_bufs.insert(buf);
        }

        dirty_blocks.clear();
        flush_lock->unlock(); // Write transactions can now proceed again.

        /* Start writing all the dirty bufs down, as a transaction. */
        typename serializer_t::write *writes =
            (typename serializer_t::write *)calloc(flush_bufs.size(),
                                                   sizeof *writes);
        int i = 0;
        for (typename std::set<buf_t *>::iterator it = flush_bufs.begin();
             it != flush_bufs.end(); ++it, i++) {
            writes[i].block_id = (*it)->get_block_id();
            writes[i].buf = (*it)->ptr();
            writes[i].callback = (*it);
        }
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
            assert(flush_bufs.find(buf) != flush_bufs.end());
            flush_bufs.erase(buf);
            buf->set_clean();
            buf->release();
        }
        if (flush_bufs.empty()) {
            /* Notify all waiting transactions of completion. */
            txn_state_t *_txn_state = flush_txns.head();
            while(_txn_state) {
                txn_state_t *_next = _txn_state->next;

                _txn_state->txn->committed(_txn_state->callback);
                
                flush_txns.remove(_txn_state);
                delete _txn_state;
                _txn_state = _next;
            }
            assert(flush_txns.empty());

            for (typename std::vector<sync_callback_t *>::iterator it =
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
            start_flush_timer();
            
            //printf("Writeback complete\n");
        } else {
            //printf("Flush bufs, waiting for %ld more\n", flush_bufs.size());
        }
    }
}

#endif  // __BUFFER_CACHE_WRITEBACK_IMPL_HPP__
