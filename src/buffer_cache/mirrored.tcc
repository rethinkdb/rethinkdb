
#ifndef __BUFFER_CACHE_MIRRORED_TCC__
#define __BUFFER_CACHE_MIRRORED_TCC__

#include "worker_pool.hpp"

/**
 * Buffer implementation.
 */
 
// This form of the buf constructor is used when the block exists on disk and needs to be loaded
template <class config_t>
buf<config_t>::buf(cache_t *cache, block_id_t block_id, block_available_callback_t *callback)
    : cache(cache),
#ifndef NDEBUG
      active_callback_count(0),
#endif
      block_id(block_id),
      cached(false),
      do_delete(false),
      writeback_buf(this),
      page_repl_buf(this),
      concurrency_buf(this),
      page_map_buf(this) {
      
    data = cache->alloc.malloc(cache->serializer.block_size);
    
#ifndef NDEBUG
    active_callback_count ++;
#endif
    cache->serializer.do_read(block_id, data, this);
    
    add_load_callback(callback);
}

// This form of the buf constructor is used when a completely new block is being created
template <class config_t>
buf<config_t>::buf(cache_t *cache)
    : cache(cache),
#ifndef NDEBUG
      active_callback_count(0),
#endif
      block_id(cache->serializer.gen_block_id()),
      cached(true),
      do_delete(false),
      writeback_buf(this),
      page_repl_buf(this),
      concurrency_buf(this),
      page_map_buf(this) {
      
    data = cache->alloc.malloc(cache->serializer.block_size);

#ifndef NDEBUG
    // The memory allocator already filled this with 0xBD, but it's nice to be able to distinguish
    // between problems with uninitialized memory and problems with uninitialized blocks
    memset(data, 0xCD, cache->serializer.block_size);
#endif
}

template <class config_t>
buf<config_t>::~buf() {
    // TODO: if we shutdown the server befire 5 seconds, the buffer hasn't had a chance to flush yet,
    // and this assert fails. Please fix.

#ifndef NDEBUG
    // We're about to free the data, let's set it to a recognizable
    // value to make sure we don't depend on accessing things that may
    // be flushed out of the cache.
    memset(data, 0xDD, cache->serializer.block_size);
#endif
    
    assert(safe_to_unload());
    cache->alloc.free(data);
}

template <class config_t>
void buf<config_t>::release() {
#ifndef NDEBUG
    cache->n_blocks_released++;
#endif
    concurrency_buf.release();

    // If the block has been marked for deletion, pass that on to the
    // serializer and unload
    if(do_delete) {
        // The FSMs should acquire and release blocks in a way that
        // makes sure a block is always safe to unload on delete.
        assert(safe_to_unload());
        cache->delete_block(block_id);
        delete this;
        return;
    }
    
    /*
    // If this code is not commented out, then it will cause bufs to be unloaded very aggressively.
    // This is useful for catching bugs in which something expects a buf to remain valid even though
    // it is eligible to be unloaded.
    if (safe_to_unload()) {
        delete this;
    }
    */
}

template <class config_t>
void buf<config_t>::on_serializer_read() {
    
    // TODO: add an assert to make sure on_serializer_read is called by the
    // same event_queue as the one on which the buf_t was created.
    
#ifndef NDEBUG
    active_callback_count --;
    assert(active_callback_count >= 0);
#endif
    
    assert(!cached);
    cached = true;
    
    // We're calling back objects that were all able to grab a lock,
    // but are waiting on a load. Because of this, we can notify all
    // of them.
    
    int n_callbacks = load_callbacks.size();
    while(n_callbacks-- > 0) {
        block_available_callback_t *callback = load_callbacks.head();
        load_callbacks.remove(callback);
        callback->on_block_available(this);
        // At this point, 'this' may be invalid because the callback might cause the block to be
        // unloaded as a side effect. That's why we use 'n_callbacks' rather than checking for
        // load_callbacks.empty().
    }
}

template <class config_t>
void buf<config_t>::on_serializer_write_block() {

#ifndef NDEBUG
    active_callback_count --;
    assert(active_callback_count >= 0);
#endif

    cache->writeback.buf_was_written(this);
}

template<class config_t>
void buf<config_t>::add_load_callback(block_available_callback_t *cb) {
    assert(!cached);
    assert(cb);
    load_callbacks.push_back(cb);
}

template<class config_t>
bool buf<config_t>::safe_to_unload() {
    return concurrency_buf.safe_to_unload() &&
        load_callbacks.empty() &&
        writeback_buf.safe_to_unload();
}

/**
 * Transaction implementation.
 */
template <class config_t>
transaction<config_t>::transaction(cache_t *cache, access_t access)
    : cache(cache),
      access(access),
      begin_callback(NULL),
      commit_callback(NULL),
      state(state_open) {
#ifndef NDEBUG
    event_queue = get_cpu_context()->event_queue;
#endif
    assert(access == rwi_read || access == rwi_write);
}

template <class config_t>
transaction<config_t>::~transaction() {
    assert(state == state_committed);
}

template <class config_t>
bool transaction<config_t>::commit(transaction_commit_callback_t *callback) {
    assert(state == state_open);
    
    cache->on_transaction_commit(this);
    
    if (access == rwi_write && cache->writeback.wait_for_flush) {
        if (cache->writeback.sync_patiently(this)) {
            state = state_committed;
            delete this;
            return true;
        
        } else {
            state = state_committing;
            commit_callback = callback;
            return false;
        }
        
    } else {
        state = state_committed;
        delete this;
        return true;
    }
}

template <class config_t>
void transaction<config_t>::on_sync() {
    assert(state == state_committing);
    state = state_committed;
    // TODO(NNW): We should push notifications through event queue.
    commit_callback->on_txn_commit(this);
    delete this;
}

template <class config_t>
typename config_t::buf_t *
transaction<config_t>::allocate(block_id_t *block_id) {

	/* Make a completely new block, complete with a shiny new block_id. */
	
    assert(event_queue == get_cpu_context()->event_queue);
#ifndef NDEBUG
    cache->n_blocks_acquired++;
#endif
    assert(access == rwi_write);
    
    // If we are getting low on memory, kick out old blocks
    cache->page_repl.make_space(1);
    
    // This form of the buf_t constructor generates a new block with a new block ID.
    buf_t *buf = new buf_t(cache);
    *block_id = buf->get_block_id();   // Find out what block ID the buf was assigned
    
    // This must pass since no one else holds references to this block.
    bool acquired __attribute__((unused)) = buf->concurrency_buf.acquire(rwi_write, NULL);
    assert(acquired);

    return buf;
}

template <class config_t>
typename config_t::buf_t *
transaction<config_t>::acquire(block_id_t block_id, access_t mode,
                               block_available_callback_t *callback) {
    assert(event_queue == get_cpu_context()->event_queue);
    assert(mode == rwi_read || access != rwi_read);
       
#ifndef NDEBUG
    cache->n_blocks_acquired++;
#endif

    buf_t *buf = cache->page_map.find(block_id);
    if (!buf) {
        
        /* Unlike in allocate(), we aren't creating a new block; the block already existed but we
        are creating a buf to represent it in memory, and then loading it from disk. */
        
        // If we are getting low on memory, kick out old blocks
        cache->page_repl.make_space(1);
        
        // This form of the buf_t constructor will instruct it to load itself from the file and
        // call us back when it finishes.
        buf = new buf_t(cache, block_id, callback);
        
        // This must pass since no one else holds references to this block.
        bool acquired __attribute__((unused)) = buf->concurrency_buf.acquire(mode, NULL);
        assert(acquired);
        
        return NULL;
        
    } else {

        bool acquired = buf->concurrency_buf.acquire(mode, callback);

        if (!acquired) {
            return NULL;
            
        } else if(!buf->is_cached()) {
            // Acquired the block, but it's not cached yet, add
            // callback to load_callbacks.
            buf->add_load_callback(callback);
            return NULL;
            
        } else {
            return buf;
        }
    }
}

/**
 * Cache implementation.
 */

template <class config_t>
mirrored_cache_t<config_t>::mirrored_cache_t(
            char *filename,
            size_t _block_size,
            size_t _max_size,
            bool wait_for_flush,
            unsigned int flush_timer_ms,
            unsigned int flush_threshold_percent) :
#ifndef NDEBUG
    n_blocks_acquired(0), n_blocks_released(0),
#endif
    serializer(
        filename,
        _block_size),
    page_repl(
        // Launch page replacement if the user-specified maximum number of blocks is reached
        _max_size / _block_size,
        this),
    writeback(
        this,
        wait_for_flush,
        flush_timer_ms,
        _max_size / _block_size * flush_threshold_percent / 100),
    shutdown_transaction_backdoor(false),
    state(state_unstarted),
    num_live_transactions(0)
    { }

template <class config_t>
mirrored_cache_t<config_t>::~mirrored_cache_t() {
    
    assert(state == state_unstarted || state == state_shut_down);
    
    while (buf_t *buf = page_repl.get_first_buf()) {
       delete buf;
    }
    assert(n_blocks_released == n_blocks_acquired);
    assert(num_live_transactions == 0);
}

template <class config_t>
bool mirrored_cache_t<config_t>::start(ready_callback_t *cb) {
    assert(state == state_unstarted);
    state = state_starting_up_start_serializer;
    ready_callback = NULL;
    if (next_starting_up_step()) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

template<class config_t>
bool mirrored_cache_t<config_t>::next_starting_up_step() {
    
    if (state == state_starting_up_start_serializer) {
        if (serializer.start(this)) {
            state = state_starting_up_finish;
        } else {
            state = state_starting_up_waiting_for_serializer;
            return false;
        }
    }
    
    if (state == state_starting_up_finish) {
        writeback.start();
        state = state_ready;
        
        if (ready_callback) ready_callback->on_cache_ready();
        ready_callback = NULL;
        
        return true;
    }
    
    fail("Invalid state.");
}

template <class config_t>
void mirrored_cache_t<config_t>::on_serializer_ready() {
    // If we received a shutdown() before we finished starting up, then the serializer should
    // not have produced this callback. Therefore we must be in the starting-up state.
    assert(state == state_starting_up_waiting_for_serializer);
    state = state_starting_up_finish;
    next_starting_up_step();
}

template <class config_t>
typename config_t::transaction_t *
mirrored_cache_t<config_t>::begin_transaction(access_t access,
        transaction_begin_callback<config_t> *callback) {\
    
    // shutdown_transaction_backdoor allows the writeback to request a transaction for the shutdown
    // sync.
    assert(state == state_ready ||
        (shutdown_transaction_backdoor &&
            (state == state_shutting_down_start_flush ||
                state == state_shutting_down_waiting_for_flush)));
    
    transaction_t *txn = new transaction_t(this, access);
    num_live_transactions ++;
    if (writeback.begin_transaction(txn, callback)) return txn;
    else return NULL;
}

template <class config_t>
void mirrored_cache_t<config_t>::on_transaction_commit(transaction_t *txn) {
    
    assert(state == state_ready ||
        state == state_shutting_down_waiting_for_transactions ||
        state == state_shutting_down_start_flush ||
        state == state_shutting_down_waiting_for_flush);
    
    writeback.on_transaction_commit(txn);
    
    num_live_transactions--;
    if (state == state_shutting_down_waiting_for_transactions && num_live_transactions == 0) {
        // We started a shutdown earlier, but we had to wait for the transactions to all finish.
        // Now that all transactions are done, continue shutting down.
        state = state_shutting_down_start_flush;
        next_shutting_down_step();
    }
}

template <class config_t>
bool mirrored_cache_t<config_t>::shutdown(shutdown_callback_t *cb) {

    assert(state == state_starting_up_waiting_for_serializer || state == state_ready);
    
    if (state == state_starting_up_waiting_for_serializer) {
        // We were shut down before we could even finish starting up. We were waiting for the
        // serializer to call us back to say it was done starting up. There is no need to flush
        // the writeback because the cache was never initialized and there cannot be any
        // dirty blocks.
        state = state_shutting_down_shutdown_serializer;
        shutdown_callback = NULL;
        if (next_shutting_down_step()) {
            return true;
        } else {
            shutdown_callback = cb;
            return false;
        }
    
    } else {
        if (num_live_transactions == 0) {
            state = state_shutting_down_start_flush;
            shutdown_callback = NULL;
            if (next_shutting_down_step()) {
                return true;
            } else {
                shutdown_callback = cb;
                return false;
            }
        } else {
            // The shutdown will be resumed by on_transaction_commit() when the last transaction
            // completes.
            state = state_shutting_down_waiting_for_transactions;
            shutdown_callback = cb;
            return false;
        }
    }
}

template<class config_t>
bool mirrored_cache_t<config_t>::next_shutting_down_step() {

    if (state == state_shutting_down_start_flush) {
        if (writeback.sync(this)) {
            state = state_shutting_down_shutdown_serializer;
        } else {
            state = state_shutting_down_waiting_for_flush;
            return false;
        }
    }
    
    if (state == state_shutting_down_shutdown_serializer) {
        if (serializer.shutdown(this)) {
            state = state_shutting_down_finish;
        } else {
            state = state_shutting_down_waiting_for_serializer;
            return false;
        }
    }
    
    if (state == state_shutting_down_finish) {
        state = state_shut_down;
        
        if (shutdown_callback) shutdown_callback->on_cache_shutdown();
        shutdown_callback = NULL;
        
        return true;
    }
    
    fail("Invalid state.");
}

template<class config_t>
void mirrored_cache_t<config_t>::on_sync() {
    assert(state == state_shutting_down_waiting_for_flush);
    state = state_shutting_down_shutdown_serializer;
    next_shutting_down_step();
}

template<class config_t>
void mirrored_cache_t<config_t>::on_serializer_shutdown() {
    assert(state == state_shutting_down_waiting_for_serializer);
    state = state_shutting_down_finish;
    next_shutting_down_step();
}

#endif  // __BUFFER_CACHE_MIRRORED_TCC__
