
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include "arch/arch.hpp"
#include "concurrency/access.hpp"
#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/mirrored/callbacks.hpp"
#include "containers/two_level_array.hpp"
#include <boost/crc.hpp>

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

/* Buffer class. */
template<class mc_config_t>
class mc_buf_t :
    public mc_config_t::serializer_t::read_callback_t,
    public mc_config_t::serializer_t::write_block_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, mc_buf_t<mc_config_t> >,
    public intrusive_list_node_t<mc_buf_t<mc_config_t> >
{
    typedef mc_cache_t<mc_config_t> cache_t;
    typedef mc_block_available_callback_t<mc_config_t> block_available_callback_t;
    
    friend class mc_cache_t<mc_config_t>;
    friend class mc_transaction_t<mc_config_t>;
    friend class mc_config_t::writeback_t;
    friend class mc_config_t::writeback_t::local_buf_t;
    friend class mc_config_t::page_repl_t;
    friend class mc_config_t::page_repl_t::local_buf_t;
    friend class mc_config_t::concurrency_t;
    friend class mc_config_t::concurrency_t::local_buf_t;
    friend class mc_config_t::page_map_t;
    friend class mc_config_t::page_map_t::local_buf_t;
    
private:
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

    /* Is the block meant to be deleted upon release? */
    bool do_delete;
    
    typedef intrusive_list_t<block_available_callback_t> callbacks_t;
    callbacks_t load_callbacks;
    
    // Each of these local buf types holds a redundant pointer to the buf that they are a part of
    typename mc_config_t::writeback_t::local_buf_t writeback_buf;
    typename mc_config_t::page_repl_t::local_buf_t page_repl_buf;
    typename mc_config_t::concurrency_t::local_buf_t concurrency_buf;
    typename mc_config_t::page_map_t::local_buf_t page_map_buf;

    // This constructor creates a buf for an existing block, and starts the process of loading it
    // from the file
    mc_buf_t(cache_t *cache, block_id_t block_id, block_available_callback_t *callback);
    
    // This constructor creates a new buf with a new block id
    explicit mc_buf_t(cache_t *cache);

    ~mc_buf_t();
    
    bool is_cached() { return cached; }

    // Callback API
    void add_load_callback(block_available_callback_t *callback);
    
    void on_serializer_read();
    void on_serializer_write_block();
    
    bool safe_to_unload();
    bool safe_to_delete();

public:
    void release();


    void *get_data_write() {
        assert(cached);
        assert(!safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
        assert(!do_delete);
        assert(sizeof(typename serializer_t::buf_data_t) == BLOCK_META_DATA_SIZE);

        writeback_buf.set_dirty();

        return ((char *) data) + BLOCK_META_DATA_SIZE;
    }

    const void *get_data_read() {
        assert(cached);
        assert(!safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
        assert(sizeof(typename mc_config_t::serializer_t::buf_data_t) == BLOCK_META_DATA_SIZE);
        return ((char *) data) + BLOCK_META_DATA_SIZE;
    }

    block_id_t get_block_id() const { return block_id; }
    
    void mark_deleted() {
        assert(!safe_to_unload());
        assert(safe_to_delete());
        do_delete = true;
        writeback_buf.set_dirty();
    }

    bool is_dirty() {
        return writeback_buf.dirty;
    }
};



/* Transaction class. */
template<class mc_config_t>
class mc_transaction_t :
    public lock_available_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, mc_transaction_t<mc_config_t> >,
    public mc_config_t::writeback_t::sync_callback_t
{
    typedef mc_cache_t<mc_config_t> cache_t;
    typedef mc_buf_t<mc_config_t> buf_t;
    typedef mc_transaction_begin_callback_t<mc_config_t> transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t<mc_config_t> transaction_commit_callback_t;
    typedef mc_block_available_callback_t<mc_config_t> block_available_callback_t;
    
    friend class mc_cache_t<mc_config_t>;
    friend class mc_config_t::writeback_t;
    
public:
    cache_t *get_cache() const { return cache; }
    access_t get_access() const { return access; }

    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);

private:
    explicit mc_transaction_t(cache_t *cache, access_t access);
    ~mc_transaction_t();

    virtual void on_lock_available() { begin_callback->on_txn_begin(this); }
    virtual void on_sync();

    cache_t *cache;
    access_t access;
    transaction_begin_callback_t *begin_callback;
    transaction_commit_callback_t *commit_callback;
    enum { state_open, state_in_commit_call, state_committing, state_committed } state;
};

template<class mc_config_t>
struct mc_cache_t :
    private mc_config_t::serializer_t::ready_callback_t,
    private mc_config_t::writeback_t::sync_callback_t,
    private mc_config_t::serializer_t::shutdown_callback_t,
    public home_cpu_mixin_t
{
    friend class mc_buf_t<mc_config_t>;
    friend class mc_transaction_t<mc_config_t>;
    friend class mc_config_t::writeback_t;
    friend class mc_config_t::writeback_t::local_buf_t;
    friend class mc_config_t::page_repl_t;
    friend class mc_config_t::page_repl_t::local_buf_t;
    friend class mc_config_t::concurrency_t;
    friend class mc_config_t::concurrency_t::local_buf_t;
    friend class mc_config_t::page_map_t;
    friend class mc_config_t::page_map_t::local_buf_t;    
    
public:
    typedef mc_buf_t<mc_config_t> buf_t;
    typedef mc_transaction_t<mc_config_t> transaction_t;
    typedef mc_block_available_callback_t<mc_config_t> block_available_callback_t;
    typedef mc_transaction_begin_callback_t<mc_config_t> transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t<mc_config_t> transaction_commit_callback_t;
    
private:
#ifndef NDEBUG
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
    typename mc_config_t::serializer_t serializer;
    typename mc_config_t::page_map_t page_map;
    typename mc_config_t::page_repl_t page_repl;
    typename mc_config_t::writeback_t writeback;
    typename mc_config_t::concurrency_t concurrency;

public:
    mc_cache_t(
            char *filename,
            size_t _block_size,
            size_t _max_size,
            bool wait_for_flush,
            unsigned int flush_timer_ms,
            unsigned int flush_threshold_percent);
    ~mc_cache_t();
    
    /* You must call start() before using the cache. If it starts up immediately, it will return
    'true'; otherwise, it will return 'false' and call 'cb' when it is done starting up.
    */

public:
    struct ready_callback_t {
        virtual void on_cache_ready() = 0;
    };
    bool start(ready_callback_t *cb);
private:
    bool next_starting_up_step();
    ready_callback_t *ready_callback;
    void on_serializer_ready();
    
public:
    
    // Transaction API
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback);
    
    /* You should call shutdown() before destroying the cache. It is safe to call shutdown() before
    the cache has finished starting up. If it shuts down immediately, it will return 'true';
    otherwise, it will return 'false' and call 'cb' when it is done starting up.
    
    It is not safe to call the cache's destructor from within on_cache_shutdown(). */
    
public:
    struct shutdown_callback_t {
        virtual void on_cache_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);
private:
    bool next_shutting_down_step();
    shutdown_callback_t *shutdown_callback;
    void on_sync();
    void on_serializer_shutdown();
    
    /* It is illegal to start new transactions during shutdown, but the writeback needs to start a
    transaction so that it can request bufs. The way this is done is that the writeback sets
    shutdown_transaction_backdoor to true before it starts the transaction and to false immediately
    afterwards. */
    bool shutdown_transaction_backdoor;
    
#ifndef NDEBUG
	// Prints debugging information designed to resolve deadlocks
	void deadlock_debug();
#endif

private:
    
    void on_transaction_commit(transaction_t *txn);
    
    enum state_t {
        state_unstarted,
        
        state_starting_up_start_serializer,
        state_starting_up_waiting_for_serializer,
        state_starting_up_finish,
        
        state_ready,
        
        state_shutting_down_waiting_for_transactions,
        state_shutting_down_start_flush,
        state_shutting_down_waiting_for_flush,
        state_shutting_down_shutdown_serializer,
        state_shutting_down_waiting_for_serializer,
        state_shutting_down_finish,
        
        state_shut_down
    } state;
    
    // Used to keep track of how many transactions there are so that we can wait for transactions to
    // complete before shutting down.
    int num_live_transactions;
private:
};

#include "mirrored.tcc"

#endif // __MIRRORED_CACHE_HPP__

