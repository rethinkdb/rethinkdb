
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include "arch/arch.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/mirrored/callbacks.hpp"
#include "containers/two_level_array.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "config/cmd_args.hpp"
#include "buffer_cache/stats.hpp"
#include <boost/crc.hpp>

#include "writeback/writeback.hpp"

#include "page_repl/page_repl_random.hpp"
typedef page_repl_random_t page_repl_t;

#include "free_list.hpp"
typedef array_free_list_t free_list_t;

#include "page_map.hpp"
typedef array_map_t page_map_t;

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

class mc_inner_buf_t {

    friend class load_buf_fsm_t;
    friend class mc_cache_t;
    friend class mc_transaction_t;
    friend class mc_buf_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class page_repl_random_t;
    friend class page_repl_random_t::local_buf_t;
    friend class array_map_t;
    friend class array_map_t::local_buf_t;
    
    typedef mc_cache_t cache_t;
    
    cache_t *cache;
    block_id_t block_id;
    void *data;
    rwi_lock_t lock;
    
    /* The number of mc_buf_ts that exist for this mc_inner_buf_t */
    int refcount;
    
    /* true if we are being deleted */
    bool do_delete;
    
    /* true if there is a mc_buf_t that holds a pointer to the data in read-only outdated-OK
    mode. */
    bool cow_will_be_needed;
    
    // Each of these local buf types holds a redundant pointer to the inner_buf that they are a part of
    writeback_t::local_buf_t writeback_buf;
    page_repl_t::local_buf_t page_repl_buf;
    page_map_t::local_buf_t page_map_buf;
    
    bool safe_to_unload();
    
    // Load an existing buf from disk
    mc_inner_buf_t(cache_t *cache, block_id_t block_id);
    
    // Create an entirely new buf
    explicit mc_inner_buf_t(cache_t *cache);
    
    ~mc_inner_buf_t();
};

/* This class represents a hold on a mc_inner_buf_t. */
class mc_buf_t :
    public lock_available_callback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_block_available_callback_t block_available_callback_t;
    
    friend class mc_cache_t;
    friend class mc_transaction_t;
    
private:
    mc_buf_t(mc_inner_buf_t *, access_t mode);
    void on_lock_available();
    bool ready;
    block_available_callback_t *callback;
    
    ticks_t start_time;
    
    access_t mode;
    mc_inner_buf_t *inner_buf;
    void *data;   /* Same as inner_buf->data until a COW happens */

    ~mc_buf_t();

public:
    void release();

    void *get_data_write() {
        assert(ready);
        assert(!inner_buf->safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
        assert(!inner_buf->do_delete);
        assert(mode == rwi_write);
        inner_buf->writeback_buf.set_dirty();
        
        assert(data == inner_buf->data);
        return data;
    }

    const void *get_data_read() {
        assert(ready);
        return data;
    }

    block_id_t get_block_id() const {
        return inner_buf->block_id;
    }
    
    void mark_deleted() {
        assert(mode == rwi_write);
        assert(!inner_buf->safe_to_unload());
        inner_buf->do_delete = true;
        inner_buf->writeback_buf.set_dirty();
    }

    bool is_dirty() {
        return inner_buf->writeback_buf.dirty;
    }
};

/* Transaction class. */
class mc_transaction_t :
    public lock_available_callback_t,
    public writeback_t::sync_callback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mc_block_available_callback_t block_available_callback_t;
    
    friend class mc_cache_t;
    friend class writeback_t;
    friend struct acquire_lock_callback_t;
    
public:
    cache_t *get_cache() const { return cache; }
    access_t get_access() const { return access; }

    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);

    cache_t *cache;

private:
    explicit mc_transaction_t(cache_t *cache, access_t access);
    ~mc_transaction_t();
    
    ticks_t start_time;
    
    virtual void on_lock_available();
    virtual void on_sync();

    void process_buf(buf_t *buf, access_t mode);
    
    access_t access;
    transaction_begin_callback_t *begin_callback;
    transaction_commit_callback_t *commit_callback;
    enum { state_open, state_in_commit_call, state_committing, state_committed } state;
};

struct mc_cache_t :
    private free_list_t::ready_callback_t,
    private writeback_t::sync_callback_t,
    public home_cpu_mixin_t
{
    friend class load_buf_fsm_t;
    friend class mc_buf_t;
    friend class mc_inner_buf_t;
    friend class mc_transaction_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class page_repl_random_t;
    friend class page_repl_random_t::local_buf_t;
    friend class array_map_t;
    friend class array_map_t::local_buf_t;    
    
public:
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_buf_t buf_t;
    typedef mc_transaction_t transaction_t;
    typedef mc_block_available_callback_t block_available_callback_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;
    
private:
    
    // TODO: how do we design communication between cache policies?
    // Should they all have access to the cache, or should they only
    // be given access to each other as necessary? The first is more
    // flexible as anyone can access anyone else, but encourages too
    // many dependencies. The second is more strict, but might not be
    // extensible when some policy implementation requires access to
    // components it wasn't originally given.
    
    translator_serializer_t *serializer;
    
    page_map_t page_map;
    page_repl_t page_repl;
    writeback_t writeback;
    free_list_t free_list;

public:
    mc_cache_t(
            translator_serializer_t *serializer,
            mirrored_cache_config_t *config);
    ~mc_cache_t();
    
    /* You must call start() before using the cache. If it starts up immediately, it will return
    'true'; otherwise, it will return 'false' and call 'cb' when it is done starting up.
    */

public:
    struct ready_callback_t {
        virtual void on_cache_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start(ready_callback_t *cb);
private:
    bool next_starting_up_step();
    ready_callback_t *ready_callback;
    void on_free_list_ready();
    
public:
    
    block_size_t get_block_size();
    
    // Transaction API
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback);
    
    /* You should call shutdown() before destroying the cache. It is safe to call shutdown() before
    the cache has finished starting up. If it shuts down immediately, it will return 'true';
    otherwise, it will return 'false' and call 'cb' when it is done starting up.
    
    It is not safe to call the cache's destructor from within on_cache_shutdown(). */
    
public:
    struct shutdown_callback_t {
        virtual void on_cache_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    bool shutdown(shutdown_callback_t *cb);
private:
    bool next_shutting_down_step();
    shutdown_callback_t *shutdown_callback;
    void on_sync();
    
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
        
        state_starting_up_create_free_list,
        state_starting_up_waiting_for_free_list,
        state_starting_up_finish,
        
        state_ready,
        
        state_shutting_down_waiting_for_transactions,
        state_shutting_down_start_flush,
        state_shutting_down_waiting_for_flush,
        state_shutting_down_finish,
        
        state_shut_down
    } state;
    
    // Used to keep track of how many transactions there are so that we can wait for transactions to
    // complete before shutting down.
    int num_live_transactions;
};

#endif // __MIRRORED_CACHE_HPP__

