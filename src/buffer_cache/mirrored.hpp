
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include "event_queue.hpp"
#include "cpu_context.hpp"
#include "concurrency/access.hpp"
#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/callbacks.hpp"

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

/* Buffer class. */
template <class config_t>
class buf : public iocallback_t,
            public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, buf<config_t> >,
            public intrusive_list_node_t<buf<config_t> >
{
    friend class config_t::cache_t;
    friend class config_t::transaction_t;
    friend class config_t::writeback_t;
    friend class config_t::writeback_t::local_buf_t;
    friend class config_t::page_repl_t;
    friend class config_t::page_repl_t::local_buf_t;
    friend class config_t::concurrency_t;
    friend class config_t::concurrency_t::local_buf_t;
    friend class config_t::page_map_t;
    friend class config_t::page_map_t::local_buf_t;
    
private:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::cache_t cache_t;
    typedef block_available_callback<config_t> block_available_callback_t;

    cache_t *cache;

#ifndef NDEBUG
    // This field helps catch bugs wherein a block is unloaded even though a callback still points
    // to it.
    int active_callback_count;
#endif
  
    block_id_t block_id;
    void *data;
    
    /* Is data valid, or are we waiting for a read? */
    bool cached;
    
    typedef intrusive_list_t<block_available_callback_t> callbacks_t;
    callbacks_t load_callbacks;
    
    // Each of these local buf types holds a redundant pointer to the buf that they are a part of
    typename config_t::writeback_t::local_buf_t writeback_buf;
    typename config_t::page_repl_t::local_buf_t page_repl_buf;
    typename config_t::concurrency_t::local_buf_t concurrency_buf;
    typename config_t::page_map_t::local_buf_t page_map_buf;

    // This constructor creates a buf for an existing block, and starts the process of loading it
    // from the file
    buf(cache_t *cache, block_id_t block_id, block_available_callback_t *callback);
    
    // This constructor creates a new buf with a new block id
    explicit buf(cache_t *cache);
    
    ~buf();
    
    bool is_cached() { return cached; }

    // Callback API
    void add_load_callback(block_available_callback_t *callback);

    virtual void on_io_complete(event_t *event);
	
	bool safe_to_unload();

public:
    void release();

    // TODO(NNW) We may want a const version of ptr() as well so the non-const
    // version can verify that the buf is writable; requires pushing const
    // through a bunch of other places (such as array_node) also, however.
    void *ptr() {
        assert(cached);
        assert(!safe_to_unload());
        return data;
    }

    block_id_t get_block_id() const { return block_id; }
    
    void set_dirty() { writeback_buf.set_dirty(); }
};

/* Transaction class. */
template <class config_t>
class transaction : public lock_available_callback_t,
                    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, transaction<config_t> >,
                    public sync_callback<config_t>
{
    friend class config_t::cache_t;
    
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    typedef block_available_callback<config_t> block_available_callback_t;
    typedef transaction_begin_callback<config_t> transaction_begin_callback_t;
    typedef transaction_commit_callback<config_t> transaction_commit_callback_t;

    cache_t *get_cache() const { return cache; }
    access_t get_access() const { return access; }

    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);

#ifndef NDEBUG
    event_queue_t *event_queue; // For asserts that we haven't changed CPU.
#endif

private:
    explicit transaction(cache_t *cache, access_t access, transaction_begin_callback_t *callback);
    ~transaction();

    virtual void on_lock_available() { begin_callback->on_txn_begin(this); }
    virtual void on_sync();

    cache_t *cache;
    access_t access;
    transaction_begin_callback_t *begin_callback;
    transaction_commit_callback_t *commit_callback;
    enum { state_open, state_committing, state_committed } state;
};

template <class config_t>
struct mirrored_cache_t
{
    friend class config_t::buf_t;
    friend class config_t::transaction_t;
    friend class config_t::writeback_t;
    friend class config_t::writeback_t::local_buf_t;
    friend class config_t::page_repl_t;
    friend class config_t::page_repl_t::local_buf_t;
    friend class config_t::concurrency_t;
    friend class config_t::concurrency_t::local_buf_t;
    friend class config_t::page_map_t;
    friend class config_t::page_map_t::local_buf_t;    

private:
#ifndef NDEBUG
    int n_trans_created, n_trans_freed;
    int n_blocks_acquired, n_blocks_released;
#endif
    
    // TODO: how do we design communication between cache policies?
    // Should they all have access to the cache, or should they only
    // be given access to each other as necessary? The first is more
    // flexible as anyone can access anyone else, but encourages too
    // many dependencies. The second is more strict, but might not be
    // extensible when some policy implementation requires access to
    // components it wasn't originally given.
    buffer_alloc_t alloc;
    typename config_t::serializer_t serializer;
    typename config_t::page_map_t page_map;
    typename config_t::page_repl_t page_repl;
    typename config_t::writeback_t writeback;
    typename config_t::concurrency_t concurrency;

public:
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::buf_t buf_t;
    
    mirrored_cache_t(
            char *filename,
            size_t _block_size,
            size_t _max_size,
            bool wait_for_flush,
            unsigned int flush_timer_ms,
            unsigned int flush_threshold_percent);
    ~mirrored_cache_t();
    
    void start() {
    	writeback.start();
    }
    void shutdown(sync_callback<config_t> *cb) {
    	writeback.shutdown(cb);
    }

    // Transaction API
    transaction_t *begin_transaction(access_t access,
        transaction_begin_callback<config_t> *callback);
    
    /* TODO: These two should probably actually be global, because block_id_t is global... */
    static const block_id_t null_block_id = config_t::serializer_t::null_block_id;
    static bool is_block_id_null(block_id_t id) {
        return config_t::serializer_t::is_block_id_null(id);
    }
    
    /* TODO: ... and we can't decide where this one belongs until we decide who is responsible for
    creating new block IDs, and figure out the relationship between the serializer and the cache
    better. */
    block_id_t get_superblock_id() {
        return serializer.get_superblock_id();
    }
};

#include "buffer_cache/mirrored.tcc"

#endif // __MIRRORED_CACHE_HPP__

