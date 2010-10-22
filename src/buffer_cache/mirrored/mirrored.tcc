#include "buffer_cache/mirrored/mirrored.hpp"

/**
 * Buffer implementation.
 */
 
// This form of the buf constructor is used when the block exists on disk and needs to be loaded
template<class mc_config_t>
mc_buf_t<mc_config_t>::mc_buf_t(cache_t *cache, block_id_t block_id, block_available_callback_t *callback)
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
      
    data = cache->serializer->malloc();
    
#ifndef NDEBUG
    active_callback_count ++;
#endif
    
    if (continue_on_cpu(cache->serializer->home_cpu, this)) on_cpu_switch();
    
    if (!cached) {
        // Can't get the buf right away, gotta go through the
        // callback...
        add_load_callback(callback);
    }
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::on_serializer_read() {
    
    cache->serializer->assert_cpu();
    assert(!cached);
    cached = true;
    if (continue_on_cpu(cache->home_cpu, this)) on_cpu_switch();
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::have_read() {
    
    cache->assert_cpu();
    
#ifndef NDEBUG
    active_callback_count --;
    assert(active_callback_count >= 0);
#endif
    
    // We're calling back objects that were all able to grab a lock,
    // but are waiting on a load. Because of this, we can notify all
    // of them.
    
    int n_callbacks = load_callbacks.size();
    while (n_callbacks-- > 0) {
        block_available_callback_t *callback = load_callbacks.head();
        load_callbacks.remove(callback);
        callback->on_block_available(this);
        // At this point, 'this' may be invalid because the callback might cause the block to be
        // unloaded as a side effect. That's why we use 'n_callbacks' rather than checking for
        // load_callbacks.empty().
    }
}

// This form of the buf constructor is used when a completely new block is being created
template<class mc_config_t>
mc_buf_t<mc_config_t>::mc_buf_t(cache_t *cache)
    : cache(cache),
#ifndef NDEBUG
      active_callback_count(0),
#endif
      block_id(cache->free_list.gen_block_id()),
      cached(true),
      do_delete(false),
      writeback_buf(this),
      page_repl_buf(this),
      concurrency_buf(this),
      page_map_buf(this) {
    
    cache->assert_cpu();
    
    data = cache->serializer->malloc();

#if !defined(NDEBUG) || defined(VALGRIND)
    // The memory allocator already filled this with 0xBD, but it's nice to be able to distinguish
    // between problems with uninitialized memory and problems with uninitialized blocks
    memset(data, 0xCD, cache->serializer->get_block_size());
#endif
}

template<class mc_config_t>
mc_buf_t<mc_config_t>::~mc_buf_t() {
    
    cache->assert_cpu();
    
#ifndef NDEBUG
    // We're about to free the data, let's set it to a recognizable
    // value to make sure we don't depend on accessing things that may
    // be flushed out of the cache.
    memset(data, 0xDD, cache->serializer->get_block_size());
#endif
    
    assert(safe_to_unload());
    cache->serializer->free(data);
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::release() {
    
    cache->assert_cpu();
    
#ifndef NDEBUG
    cache->n_blocks_released++;
#endif
    concurrency_buf.release();
    
    /*
    // If this code is not commented out, then it will cause bufs to be unloaded very aggressively.
    // This is useful for catching bugs in which something expects a buf to remain valid even though
    // it is eligible to be unloaded.
    
    if (safe_to_unload()) {
        delete this;
    }
    */
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::on_serializer_write_block() {
    
    cache->serializer->assert_cpu();
    assert(is_dirty());
    if (continue_on_cpu(cache->home_cpu, this)) on_cpu_switch();
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::on_cpu_switch() {

    if (!is_cached()) {
        /* We are going to the serializer's CPU to ask it to load us. */
        cache->serializer->assert_cpu();
        if (cache->serializer->do_read(cache->get_ser_block_id(block_id), data, this)) on_serializer_read();
    
    } else if (!is_dirty()) {
        /* We are returning from the serializer's CPU after loading our data. */
        cache->assert_cpu();
        have_read();
    
    } else {
        /* We are returning from the serializer's CPU after getting 
        on_serializer_write_block() */
        cache->assert_cpu();
#ifndef NDEBUG
        active_callback_count --;
        assert(active_callback_count >= 0);
#endif
        cache->writeback.buf_was_written(this);
    }
}

template<class mc_config_t>
void mc_buf_t<mc_config_t>::add_load_callback(block_available_callback_t *cb) {
    assert(!cached);
    assert(cb);
    load_callbacks.push_back(cb);
}

template<class mc_config_t>
bool mc_buf_t<mc_config_t>::safe_to_unload() {
    return concurrency_buf.safe_to_unload() &&
        load_callbacks.empty() &&
        writeback_buf.safe_to_unload();
}

template<class mc_config_t>
bool mc_buf_t<mc_config_t>::safe_to_delete() {
    return load_callbacks.empty();
}

/**
 * Transaction implementation.
 */
template<class mc_config_t>
mc_transaction_t<mc_config_t>::mc_transaction_t(cache_t *cache, access_t access)
    : cache(cache),
      access(access),
      begin_callback(NULL),
      commit_callback(NULL),
      state(state_open) {
    assert(access == rwi_read || access == rwi_write);
}

template<class mc_config_t>
mc_transaction_t<mc_config_t>::~mc_transaction_t() {
    assert(state == state_committed);
}

template<class mc_config_t>
bool mc_transaction_t<mc_config_t>::commit(transaction_commit_callback_t *callback) {
    assert(state == state_open);
    
    /* We have to call sync_patiently() before on_transaction_commit() so that if
    on_transaction_commit() starts a sync, we will get included in it */
    if (access == rwi_write && cache->writeback.wait_for_flush) {
        if (cache->writeback.sync_patiently(this)) {
            state = state_committed;
        } else {
            state = state_in_commit_call;
        }
    } else {
        state = state_committed;
    }
    
    cache->on_transaction_commit(this);
    
    if (state == state_in_commit_call) {
        state = state_committing;
        commit_callback = callback;
        return false;
    } else {
        delete this;
        return true;
    }
}

template<class mc_config_t>
void mc_transaction_t<mc_config_t>::on_sync() {
    /* cache->on_transaction_commit() could cause on_sync() to be called even after sync_patiently()
    failed. To detect when this happens, we use the state state_in_commit_call. If we get an
    on_sync() while in state_in_commit_call, we know that we are still inside of commit(), so we
    don't delete ourselves yet and just set state to state_committed instead, thereby signalling
    commit() to delete us. I think there must be a better way to do this, but I can't think of it
    right now. */
    
    if (state == state_in_commit_call) {
        state = state_committed;
    } else if (state == state_committing) {
        state = state_committed;
        // TODO(NNW): We should push notifications through event queue.
        commit_callback->on_txn_commit(this);
        delete this;
    } else {
        fail("Unexpected state.");
    }
}

template<class mc_config_t>
mc_buf_t<mc_config_t> *mc_transaction_t<mc_config_t>::allocate(block_id_t *block_id) {

    /* Make a completely new block, complete with a shiny new block_id. */
    
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

template<class mc_config_t>
mc_buf_t<mc_config_t> *mc_transaction_t<mc_config_t>::acquire(block_id_t block_id, access_t mode,
                               block_available_callback_t *callback) {
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

        // It's possible that we got the block from the serializer
        // right way because of its internal caching. If that's the
        // case, just return it right away.
        if(buf->is_cached()) {
            return buf;
        } else {
            return NULL;
        }
        
    } else {
        
        assert(!buf->do_delete);
        
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

template<class mc_config_t>
mc_cache_t<mc_config_t>::mc_cache_t(
            serializer_t *serializer,
            int id_on_serializer,
            int count_on_serializer,
            size_t _max_size,
            bool wait_for_flush,
            unsigned int flush_timer_ms,
            unsigned int flush_threshold_percent) :
#ifndef NDEBUG
    n_blocks_acquired(0), n_blocks_released(0),
#endif
    serializer(serializer),
    id_on_serializer(id_on_serializer),
    count_on_serializer(count_on_serializer),
    page_repl(
        // Launch page replacement if the user-specified maximum number of blocks is reached
        _max_size / serializer->get_block_size(),
        this),
    writeback(
        this,
        wait_for_flush,
        flush_timer_ms,
        _max_size / serializer->get_block_size() * flush_threshold_percent / 100),
    free_list(this),
    shutdown_transaction_backdoor(false),
    state(state_unstarted),
    num_live_transactions(0)
    { }

template<class mc_config_t>
mc_cache_t<mc_config_t>::~mc_cache_t() {
    
    assert(state == state_unstarted || state == state_shut_down);
    
    while (buf_t *buf = page_repl.get_first_buf()) {
       delete buf;
    }
    assert(n_blocks_released == n_blocks_acquired);
    assert(num_live_transactions == 0);
}

template<class mc_config_t>
bool mc_cache_t<mc_config_t>::start(ready_callback_t *cb) {
    assert(state == state_unstarted);
    state = state_starting_up_create_free_list;
    ready_callback = NULL;
    if (next_starting_up_step()) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

template<class mc_config_t>
bool mc_cache_t<mc_config_t>::next_starting_up_step() {
    
    if (state == state_starting_up_create_free_list) {
        if (free_list.start(this)) {
            state = state_starting_up_finish;
        } else {
            state = state_starting_up_waiting_for_free_list;
            return false;
        }
    }
    
    if (state == state_starting_up_finish) {
        
        /* Create an initial superblock */
        if (free_list.num_blocks_in_use == 0) {
        
            buf_t *b = new mc_buf_t<mc_config_t>(this);
#ifndef NDEBUG
            n_blocks_acquired++;   // Because release() increments n_blocks_released
#endif
            b->concurrency_buf.acquire(rwi_write, NULL);
            assert(b->get_block_id() == SUPERBLOCK_ID);
            bzero(b->get_data_write(), get_block_size());
            b->release();
        }
        
        state = state_ready;
        
        if (ready_callback) ready_callback->on_cache_ready();
        ready_callback = NULL;
        
        return true;
    }
    
    fail("Invalid state.");
}

template<class mc_config_t>
void mc_cache_t<mc_config_t>::on_free_list_ready() {
    assert(state == state_starting_up_waiting_for_free_list);
    state = state_starting_up_finish;
    next_starting_up_step();
}

template<class mc_config_t>
size_t mc_cache_t<mc_config_t>::get_block_size() {
    return serializer->get_block_size();
}

template<class mc_config_t>
mc_transaction_t<mc_config_t> *mc_cache_t<mc_config_t>::begin_transaction(access_t access,
        transaction_begin_callback_t *callback) {\
    
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

template<class mc_config_t>
ser_block_id_t mc_cache_t<mc_config_t>::get_ser_block_id(block_id_t block_id) {
    
    return block_id * count_on_serializer + id_on_serializer;
}

template<class mc_config_t>
void mc_cache_t<mc_config_t>::on_transaction_commit(transaction_t *txn) {
    
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

template<class mc_config_t>
bool mc_cache_t<mc_config_t>::shutdown(shutdown_callback_t *cb) {

    assert(state == state_ready);

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

template<class mc_config_t>
bool mc_cache_t<mc_config_t>::next_shutting_down_step() {

    if (state == state_shutting_down_start_flush) {
        if (writeback.sync(this)) {
            state = state_shutting_down_finish;
        } else {
            state = state_shutting_down_waiting_for_flush;
            return false;
        }
    }
    
    if (state == state_shutting_down_finish) {
        
        /* Use do_later() rather than calling it immediately because it might call
        our destructor, and it might not be safe to call our destructor right here. */
        if (shutdown_callback) do_later(shutdown_callback, &shutdown_callback_t::on_cache_shutdown);
        shutdown_callback = NULL;
        state = state_shut_down;
        
        return true;
    }
    
    fail("Invalid state.");
}

template<class mc_config_t>
void mc_cache_t<mc_config_t>::on_sync() {
    assert(state == state_shutting_down_waiting_for_flush);
    state = state_shutting_down_finish;
    next_shutting_down_step();
}

