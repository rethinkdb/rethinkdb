
#ifndef __BUFFER_CACHE_MIRRORED_IMPL_HPP__
#define __BUFFER_CACHE_MIRRORED_IMPL_HPP__

/**
 * Buffer implementation.
 */
template <class config_t>
buf<config_t>::buf(transaction_t *transaction, block_id_t block_id)
    : config_t::writeback_t::buf_t(transaction->get_cache()),
      config_t::page_repl_t::buf_t(transaction->get_cache()),
      config_t::concurrency_t::buf_t(),
    transaction(transaction),
    cache(transaction->get_cache()),
    block_id(block_id),
    cached(false) {
    data = cache->malloc(((serializer_t *)cache)->block_size);
}

template <class config_t>
buf<config_t>::~buf() {
    cache->free(data);
}

template <class config_t>
void buf<config_t>::release(void *state) {
    /* XXX vvv This is incorrect. */
    if (this->is_dirty()) {
        cache->do_write(get_cpu_context()->event_queue, block_id, ptr(), this);
        this->set_clean(); /* XXX XXX Can't do this until the I/O comes back! */
    }
    /* XXX ^^^ This is incorrect. */
    if (!this->is_dirty())
        this->unpin();
    ((concurrency_t *)cache)->release(this);

    // TODO: pinning/unpinning a block should come implicitly from
    // concurrency_t because it maintains all relevant reference
    // counts.
}

template <class config_t>
void buf<config_t>::add_lock_callback(block_available_callback_t *callback) {
    printf("Waiting on a lock: %p\n", this);
    if(callback)
        lock_callbacks.push(callback);
}

template <class config_t>
void buf<config_t>::add_load_callback(block_available_callback_t *callback) {
    printf("Waiting on a load: %p\n", this);
    if(callback)
        load_callbacks.push(callback);
}

template <class config_t>
void buf<config_t>::notify_on_lock() {
    // We're calling back objects that were waiting on a lock. Because
    // of that, we can only call one.
    if(!lock_callbacks.empty()) {
        printf("Done waiting on a lock: %p\n", this);
        lock_callbacks.front()->on_block_available(this);
        lock_callbacks.pop();
    }
}

template <class config_t>
void buf<config_t>::notify_on_load() {
    // We're calling back objects that were all able to grab a lock,
    // but are waiting on a load. Because of this, we can notify all
    // of them.
    while(!load_callbacks.empty()) {
        printf("Done waiting on a load: %p\n", this);
        load_callbacks.front()->on_block_available(this);
        load_callbacks.pop();
    }
}

template <class config_t>
typename config_t::node_t *buf<config_t>::buf::node() {
    assert(data);
    return (typename config_t::node_t *)data;
}

/**
 * Transaction implementation.
 */
template <class config_t>
transaction<config_t>::transaction(cache_t *cache)
    : cache(cache), open(true) {
#ifndef NDEBUG
    event_queue = get_cpu_context()->event_queue;
#endif
}

template <class config_t>
transaction<config_t>::~transaction() {
    assert(!open);
}

template <class config_t>
bool transaction<config_t>::commit(void *state) {
    /* TODO(NNW): Implement!! */
    bool res = true; //cache->commit(this, state);
    if (res)
        open = false;
    return res;
}

template <class config_t>
typename config_t::buf_t *
transaction<config_t>::allocate(block_id_t *block_id) {
    assert(event_queue == get_cpu_context()->event_queue);
        
    *block_id = cache->gen_block_id();
    buf_t *buf = new buf_t(this, *block_id);
    buf->set_cached(true);
    cache->set(*block_id, buf);
    buf->pin();

    // This must pass since no one else holds references to this
    // block.
    bool acquired __attribute__((unused)) = ((concurrency_t*)cache)->acquire(buf, rwi_write, NULL);
    assert(acquired);
        
    return buf;
}

template <class config_t>
typename config_t::buf_t *
transaction<config_t>::acquire(block_id_t block_id, access_t mode,
                               block_available_callback_t *callback) {
    assert(event_queue == get_cpu_context()->event_queue);
       
    // TODO(NNW): we might get a request for a block id while the block
    // with that block id is still loading (consider two requests
    // in a row). We need to keep track of this so we don't
    // unnecessarily double IO and/or lose memory.

    buf_t *buf = (buf_t *)cache->find(block_id);
    if (!buf) {
        buf_t *buf;

        buf = new buf_t(this, block_id);
        
        // This must pass since no one else holds references to this block.
        bool acquired __attribute__((unused)) = ((concurrency_t*)cache)->acquire(buf, mode, NULL);
        assert(acquired);
        
        assert(buf->ptr()); /* XXX */
        cache->set(block_id, buf);

        buf->add_load_callback(callback);

        cache->do_read(get_cpu_context()->event_queue, block_id, buf->ptr(), buf);
    } else {
        buf->pin();

        bool acquired = ((concurrency_t*)cache)->acquire(buf, mode, buf);

        if (!acquired) {
            // Could not acquire block because of locking, add
            // callback to lock_callbacks.
            buf->add_lock_callback(callback);
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
    for (typename page_map_t::ft_map_t::iterator it = ft_map.begin();
         it != ft_map.end(); ++it) {
        buf_t *buf = (*it).second;
        bool acquired = ((concurrency_t *)this)->acquire(buf, rwi_write, NULL);
        assert(acquired); // TODO: This should be an RASSERT().
        delete buf;
    }
}

template <class config_t>
typename config_t::transaction_t *
mirrored_cache_t<config_t>::begin_transaction() {
    transaction_t *txn = new transaction_t(this);
    return txn;
}

template <class config_t>
void mirrored_cache_t<config_t>::aio_complete(buf_t *buf,
        void *block, bool written) {
    // TODO: add an assert to make sure aio_complete is called by the
    // same event_queue as the one on which the buf_t was created.

    buf->set_cached(true);

    if(written) {
        printf("Written: %p\n", buf);
        buf->unpin();
    } else {
        printf("Loaded: %p\n", buf);
        buf->pin();
        buf->notify_on_load();
    }
}

#endif  // __BUFFER_CACHE_MIRRORED_IMPL_HPP__
