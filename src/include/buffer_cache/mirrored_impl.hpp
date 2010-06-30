
#ifndef __BUFFER_CACHE_MIRRORED_IMPL_HPP__
#define __BUFFER_CACHE_MIRRORED_IMPL_HPP__

/**
 * Buffer implementation.
 */
template <class config_t>
buf<config_t>::buf(cache_t *cache, block_id_t block_id)
    : config_t::writeback_t::local_buf_t(cache),
      config_t::page_repl_t::local_buf_t(cache),
      config_t::concurrency_t::local_buf_t(this),
    cache(cache),
    block_id(block_id),
    cached(false) {
    data = cache->malloc(((serializer_t *)cache)->block_size);
}

template <class config_t>
buf<config_t>::~buf() {
    cache->free(data);
}

template <class config_t>
void buf<config_t>::release() {
#ifndef NDEBUG
    cache->n_blocks_released++;
#endif
    this->unpin();
    ((concurrency_t *)cache)->release(this);

    // TODO: pinning/unpinning a block should come implicitly from
    // concurrency_t because it maintains all relevant reference
    // counts.
}

template <class config_t>
void buf<config_t>::add_load_callback(block_available_callback_t *callback) {
    if(callback)
        load_callbacks.push(callback);
}

template <class config_t>
void buf<config_t>::notify_on_load() {
    // We're calling back objects that were all able to grab a lock,
    // but are waiting on a load. Because of this, we can notify all
    // of them.
    while(!load_callbacks.empty()) {
        load_callbacks.front()->on_block_available(this);
        load_callbacks.pop();
    }
}

template <class config_t>
void buf<config_t>::on_io_complete(event_t *event) {
    // Let the cache know about the disk action
    cache->aio_complete(this, event->op != eo_read);
}

/**
 * Transaction implementation.
 */
template <class config_t>
transaction<config_t>::transaction(cache_t *cache, access_t access,
        transaction_begin_callback_t *callback)
    : cache(cache), access(access), state(state_open) {
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
    bool res = cache->commit(this, callback);
    state = res ? state_committed : state_committing;
    if (res)
        delete this;
    return res;
}

template <class config_t>
void transaction<config_t>::committed(transaction_commit_callback_t *callback) {
    assert(state == state_committing);
    state = state_committed;
    // TODO(NNW): We should push notifications through event queue.
    callback->on_txn_commit(this);
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
        
    *block_id = cache->gen_block_id();
    buf_t *buf = new buf_t(cache, *block_id);
    buf->set_cached(true);
    cache->set(*block_id, buf);
    buf->pin();

    // This must pass since no one else holds references to this
    // block.
    bool acquired __attribute__((unused)) =
        ((concurrency_t*)cache)->acquire(buf, rwi_write, NULL);
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

    buf_t *buf = (buf_t *)cache->find(block_id);
    if (!buf) {
    	
    	/* Unlike in allocate(), we aren't creating a new block; the block already existed but we
    	are creating a buf to represent it in memory, and then loading it from disk. */
    	
    	// TODO: It's a little bit odd that the logic for loading blocks is here, but the logic for
    	// unloading blocks is in cache_t.
    	
        buf_t *buf = new buf_t(cache, block_id);
        buf->pin();
        
        // This must pass since no one else holds references to this block.
        bool acquired __attribute__((unused)) =
            ((concurrency_t*)cache)->acquire(buf, mode, NULL);
        assert(acquired);
        
        cache->set(block_id, buf);

        buf->add_load_callback(callback);

        cache->do_read(get_cpu_context()->event_queue, block_id, buf->ptr(), buf);
    } else {
        buf->pin();

        bool acquired = ((concurrency_t*)cache)->acquire(buf, mode, buf);

        if (!acquired) {
            // Could not acquire block because of locking, add
            // callback to lock_callbacks.
            ((typename config_t::concurrency_t::buf_t*)buf)->add_lock_callback(callback);
        } else if(!buf->is_cached()) {
            // Acquired the block, but it's not cached yet, add
            // callback to load_callbacks.
            buf->add_load_callback(callback);
        }

        if (!acquired || !buf->is_cached()) {
            buf = NULL;
        }
    }

    return buf;
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
        buf->unpin();
        writeback_t::aio_complete(buf, written);
    } else {
        buf->set_cached(true);
        buf->notify_on_load();
    }
}

template <class config_t>
void mirrored_cache_t<config_t>::do_unload_buf(buf_t *buf) {
	bool acquired __attribute__((unused)) =
		((concurrency_t *)this)->acquire(buf, rwi_write, NULL);
	assert(acquired);
	assert(!buf->is_dirty());
	assert(!buf->is_pinned());
	
	// Inform the page map that the block in question no longer exists
	erase(buf->get_block_id());
	
	delete buf;
}

#endif  // __BUFFER_CACHE_MIRRORED_IMPL_HPP__
