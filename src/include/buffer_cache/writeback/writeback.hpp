
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include "concurrency/rwi_lock.hpp"
#include "utils.hpp"

// TODO: What about safety_timer_ms=0 (flush on every transaction)?

template <class config_t>
struct writeback_tmpl_t : public lock_available_callback_t {
public:
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    
    writeback_tmpl_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int safety_timer_ms,
        unsigned int force_flush_threshold);
    virtual ~writeback_tmpl_t();

    void start(); // Start the writeback process as part of database startup.
    void shutdown(sync_callback<config_t> *callback);

    void sync(sync_callback<config_t> *callback);

    bool begin_transaction(transaction_t *txn);
    bool commit(transaction_t *transaction, transaction_commit_callback<config_t> *callback);
    void aio_complete(buf_t *, bool written);

#ifndef NDEBUG
    // Print debugging information designed to resolve a deadlock
    void deadlock_debug();
#endif
    
    unsigned int num_dirty_blocks();
    
    class local_buf_t : public intrusive_list_node_t<buf_t> {
    public:
        explicit local_buf_t(writeback_tmpl_t *wb)
            : writeback(wb), dirty(false) {}

        bool is_dirty() const { return dirty; }
        
        // The argument to set_dirty() is actually 'this'; local_buf_t is a mixin for buf_t, but it
        // doesn't know that, so we need to pass a pointer in buf_t form.
        void set_dirty(buf_t *buf);

        void set_clean() {
            assert(dirty);
            dirty = false;
        }

    private:
        writeback_tmpl_t *writeback;
        bool dirty;
    };

private:
    typedef typename config_t::serializer_t serializer_t;
    typedef transaction_commit_callback<config_t> transaction_commit_callback_t;
    typedef sync_callback<config_t> sync_callback_t;
    struct txn_state_t : public intrusive_list_node_t<txn_state_t>,
                         public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txn_state_t>
    {
        txn_state_t(transaction_t *_txn, transaction_commit_callback_t *_callback)
            : txn(_txn), callback(_callback)
            {}
        transaction_t *txn;
        transaction_commit_callback_t *callback;
    };

    enum state {
        state_none,
        state_locking,
        state_locked,
        state_write_bufs,
    };
    
    // The writeback system has a mechanism to keep data safe if the server crashes. If modified
    // data sits in memory for longer than safety_timer_ms milliseconds, a writeback will be
    // automatically started to store it on disk. safety_timer is the timer to keep track of how
    // much longer the data can sit in memory.
    event_queue_t::timer_t *safety_timer;
    static void safety_timer_callback(void *ctx);

    virtual void on_lock_available();
    void writeback(buf_t *buf);

    /* User-controlled settings. */
    bool wait_for_flush;
    int safety_timer_ms;
    unsigned int force_flush_threshold;

    /* Internal variables used at all times. */
    cache_t *cache;
    unsigned int num_txns;
    rwi_lock_t *flush_lock;
    // List of transactions to notify the next time writeback completes
    intrusive_list_t<txn_state_t> txns;
    // List of bufs that are currenty dirty
    intrusive_list_t<buf_t> dirty_bufs;
    std::vector<sync_callback_t*, gnew_alloc<sync_callback_t*> > sync_callbacks;
    sync_callback_t *shutdown_callback;
    bool final_sync;

    /* Internal variables used only during a flush operation. */
    enum state state;
    transaction_t *transaction;
    intrusive_list_t<txn_state_t> flush_txns;
    intrusive_list_t<buf_t> flush_bufs;
};

#include "buffer_cache/writeback/writeback_impl.hpp"

#endif // __buffer_cache_writeback_hpp__

