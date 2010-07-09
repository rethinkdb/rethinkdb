
#ifndef __BUFFER_CACHE_MIRRORED_TCC__
#define __BUFFER_CACHE_MIRRORED_TCC__

/**
 * Buffer implementation.
 */
template <class config_t>
buf<config_t>::buf(cache_t *cache, block_id_t block_id)
    : config_t::writeback_t::local_buf_t(cache),
      config_t::concurrency_t::local_buf_t(this),
    temporary_pinned(0),
    cache(cache),
    block_id(block_id),
    cached(false) {
    data = cache->malloc(((serializer_t *)cache)->block_size);
}

template <class config_t>
buf<config_t>::~buf() {
    assert(load_callbacks.empty());
    assert(temporary_pinned == 0);
    cache->free(data);
}

template <class config_t>
void buf<config_t>::release() {
#ifndef NDEBUG
    cache->n_blocks_released++;
#endif
    ((concurrency_t *)cache)->release(this);
}

template <class config_t>
void buf<config_t>::add_load_callback(block_available_callback_t *callback) {
    if(callback)
        load_callbacks.push_back(callback);
}

template <class config_t>
void buf<config_t>::notify_on_load() {

    /* We guarantee that the block will not be unloaded until all of the load callbacks return. */
    temporary_pinned ++;

    // We're calling back objects that were all able to grab a lock,
    // but are waiting on a load. Because of this, we can notify all
    // of them.
    while(!load_callbacks.empty()) {
        block_available_callback_t *_callback = load_callbacks.head();
        load_callbacks.remove(_callback);
        _callback->on_block_available(this);
    }
    
    temporary_pinned --;
}

template <class config_t>
void buf<config_t>::on_io_complete(event_t *event) {
    // Let the cache know about the disk action
    cache->aio_complete(this, event->op != eo_read);
}

template<class config_t>
bool buf<config_t>::is_pinned() {
    // TODO: Include is_dirty() in is_pinned(). Rename is_pinned() something like can_be_unloaded().
    return concurrency_t::local_buf_t::is_pinned() || !load_callbacks.empty() || temporary_pinned;
}

#ifndef NDEBUG
template<class config_t>
void buf<config_t>::deadlock_debug() {
	printf("buffer %p (id %d) {\n", (void*)this, (int)block_id);
	printf("\tdirty = %d\n", (int)config_t::writeback_t::local_buf_t::is_dirty());
	printf("\tcached = %d\n", (int)is_cached());
	concurrency_t::local_buf_t::deadlock_debug();
	printf("}\n");
}
#endif

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
    bool res = cache->commit(this);
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
    cache->make_space(1);
    
    *block_id = cache->gen_block_id();
    buf_t *buf = new buf_t(cache, *block_id);
    buf->set_cached(true);
    cache->set(*block_id, buf);
		
    // This must pass since no one else holds references to this
    // block.
    bool acquired __attribute__((unused)) =
        ((concurrency_t*)cache)->acquire(buf, rwi_write, NULL);
    assert(acquired);

	// For debugging purposes. The memory allocator will have filled it with 0xBD, but we need more
	// information than that.
	memset(buf->ptr(), 0xCD, cache->serializer_t::block_size);

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

    buf_t *buf = (buf_t *)cache->find(block_id);
    if (!buf) {
    	
    	/* Unlike in allocate(), we aren't creating a new block; the block already existed but we
    	are creating a buf to represent it in memory, and then loading it from disk. */
    	
    	// If we are getting low on memory, kick out old blocks
        cache->make_space(1);
    	
    	// TODO: It's a little bit odd that the logic for loading blocks is here, but the logic for
    	// unloading blocks is in cache_t.
    	
        buf = new buf_t(cache, block_id);
        
        // This must pass since no one else holds references to this block.
        bool acquired __attribute__((unused)) =
            ((concurrency_t*)cache)->acquire(buf, mode, NULL);
        assert(acquired);
        
        // Make an entry in the page map
        cache->set(block_id, buf);
        
		// Since the buf isn't in memory yet, we're going to start an asynchronous load request and
		// then call the callback when the load finishes. 
        buf->add_load_callback(callback);
        
        // The callback when this completes will be sent to cache_t::aio_complete(), and from there
        // to buf_t::notify_on_load(), and then to the client trying to acquire the block.
        cache->do_read(get_cpu_context()->event_queue, block_id, buf->ptr_possibly_uncached(), buf);
        
        return NULL;
        
    } else {

        bool acquired = ((concurrency_t*)cache)->acquire(buf, mode, buf);

        if (!acquired) {
            // Could not acquire block because of locking, add
            // callback to lock_callbacks.
            ((typename config_t::concurrency_t::buf_t*)buf)->add_lock_callback(callback);
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
mirrored_cache_t<config_t>::~mirrored_cache_t() {
	
	while (!ft_map.empty()) {
		buf_t *buf = ft_map.begin()->second;
		do_unload_buf(buf);
	}
    assert(n_blocks_released == n_blocks_acquired);
    assert(n_trans_created == n_trans_freed);
    assert(ft_map.empty());
    serializer_t::close();
}

template <class config_t>
typename config_t::transaction_t *
mirrored_cache_t<config_t>::begin_transaction(access_t access,
        transaction_begin_callback<config_t> *callback) {
    transaction_t *txn = new transaction_t(this, access, callback);
#ifndef NDEBUG
    n_trans_created++;
#endif

    if (writeback_t::begin_transaction(txn))
        return txn;
    return NULL;
}

template <class config_t>
void mirrored_cache_t<config_t>::aio_complete(buf_t *buf, bool written) {
    // TODO: add an assert to make sure aio_complete is called by the
    // same event_queue as the one on which the buf_t was created.

    if(written) {
        writeback_t::aio_complete(buf, written);
    } else {
        buf->set_cached(true);
        buf->notify_on_load();
    }
}

template <class config_t>
void mirrored_cache_t<config_t>::do_unload_buf(buf_t *buf) {
    assert(!buf->is_pinned());
	assert(!buf->is_dirty());
	
	// Inform the page map that the block in question no longer exists, and will have to be reloaded
	// from disk the next time it is used
	erase(buf->get_block_id());
	
	delete buf;
}

#ifndef NDEBUG
template<class config_t>
void mirrored_cache_t<config_t>::deadlock_debug() {
	
	printf("\n----- Cache %p -----\n", (void*)this);
	
	writeback_t::deadlock_debug();
	
	printf("\n----- Buffers -----\n");
	typename page_map_t::ft_map_t::iterator it;
	for (it = ft_map.begin(); it != ft_map.end(); it++) {
		buf_t *buf = it->second;
		buf->deadlock_debug();
	}
}
#endif

#endif  // __BUFFER_CACHE_MIRRORED_TCC__
