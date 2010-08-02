
#ifndef __BUFFER_CACHE_MIRRORED_TCC__
#define __BUFFER_CACHE_MIRRORED_TCC__

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
      writeback_buf(this),
      page_repl_buf(this),
      concurrency_buf(this),
      page_map_buf(this) {
      
    data = cache->alloc.malloc(cache->serializer.block_size);
    
#ifndef NDEBUG
    active_callback_count ++;
#endif
    cache->serializer.do_read(get_cpu_context()->event_queue, block_id, data, this);
    
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
    assert(safe_to_unload());
    cache->alloc.free(data);
}

template <class config_t>
void buf<config_t>::release() {
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

template <class config_t>
void buf<config_t>::on_io_complete(event_t *event) {
    
    // TODO: add an assert to make sure on_io_complete is called by the
    // same event_queue as the one on which the buf_t was created.
    
#ifndef NDEBUG
    active_callback_count --;
    assert(active_callback_count >= 0);
#endif
    
    if (event->op == eo_read) {
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
        
    } else {
        cache->writeback.buf_was_written(this);
    }
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
transaction<config_t>::transaction(cache_t *cache, access_t access,
        transaction_begin_callback_t *callback)
    : cache(cache),
      access(access),
      commit_callback(NULL),
      state(state_open) {
#ifndef NDEBUG
    event_queue = get_cpu_context()->event_queue;
#endif
    assert(access == rwi_read || access == rwi_write);
    begin_callback = callback;
}

template <class config_t>
transaction<config_t>::~transaction() {
#ifndef NDEBUG
    cache->n_trans_freed++;
#endif
    assert(state == state_committed);
}

template <class config_t>
bool transaction<config_t>::commit(transaction_commit_callback_t *callback) {
    assert(state == state_open);
    assert(!commit_callback);
    commit_callback = callback;
    bool res = cache->writeback.commit(this);
    state = res ? state_committed : state_committing;
    if (res) delete this;
    return res;
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
    n_trans_created(0), n_trans_freed(0),
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
        _max_size / _block_size * flush_threshold_percent / 100)
    { }

template <class config_t>
mirrored_cache_t<config_t>::~mirrored_cache_t() {
    
    while (buf_t *buf = page_repl.get_first_buf()) {
       delete buf;
    }
    assert(n_blocks_released == n_blocks_acquired);
    assert(n_trans_created == n_trans_freed);
}

template <class config_t>
typename config_t::transaction_t *
mirrored_cache_t<config_t>::begin_transaction(access_t access,
        transaction_begin_callback<config_t> *callback) {
    transaction_t *txn = new transaction_t(this, access, callback);
#ifndef NDEBUG
    n_trans_created++;
#endif

    if (writeback.begin_transaction(txn))
        return txn;
    return NULL;
}

#endif  // __BUFFER_CACHE_MIRRORED_TCC__
