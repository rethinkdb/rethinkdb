
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include "concurrency/rwi_lock.hpp"
#include "utils.hpp"

// TODO: What about flush_timer_ms=0 (flush on every transaction)?

template <class config_t>
struct writeback_tmpl_t : public lock_available_callback_t {
public:
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    
    writeback_tmpl_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold);
    virtual ~writeback_tmpl_t();

    void start();
    void shutdown(sync_callback<config_t> *callback);
    
    /* Forces a writeback to happen soon. Calls 'callback' back as soon as the next complete
    writeback cycle is over. */
    void sync(sync_callback<config_t> *callback);

    bool begin_transaction(transaction_t *txn);
    bool commit(transaction_t *transaction);
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
    typedef sync_callback<config_t> sync_callback_t;

    enum state {
        state_none,
        state_locking,
        state_locked,
        state_write_bufs,
    };
    
    // The writeback system has a mechanism to keep data safe if the server crashes. If modified
    // data sits in memory for longer than flush_timer_ms milliseconds, a writeback will be
    // automatically started to store it on disk. flush_timer is the timer to keep track of how
    // much longer the data can sit in memory.
    event_queue_t::timer_t *flush_timer;
    static void flush_timer_callback(void *ctx);

    virtual void on_lock_available();
    void writeback(buf_t *buf);

    /* User-controlled settings. */
    
    bool wait_for_flush;
    int flush_timer_ms;
    unsigned int flush_threshold;   // Number of blocks, not percentage

    /* Internal variables used at all times. */
    
    cache_t *cache;
    unsigned int num_txns;

    /* The flush lock is necessary because if we acquire dirty blocks
     * in random order during the flush, there might be a deadlock
     * with set_fsm. When the flush is initiated, the flush_lock is
     * grabbed, all transactions are drained, and no new transaction
     * are allowed in the meantime. Once all transactions are drained,
     * the writeback code locks all dirty blocks for reading and
     * releases the flush lock. */
    rwi_lock_t *flush_lock;
    
    // List of things waiting for their data to be written to disk. They will be called back after
    // the next complete writeback cycle completes.
    intrusive_list_t<sync_callback_t> sync_callbacks;
    
    // If something requests a sync but a sync is already in progress, then
    // start_next_sync_immediately is set so that a new sync operation is started as soon as the
    // old one finishes. Note that start_next_sync_immediately being true is not equivalent to
    // sync_callbacks being nonempty, because when wait_for_flush is set, transactions will sit
    // patiently in sync_callbacks without setting start_next_sync_immediately.
    bool start_next_sync_immediately;
    
    // List of bufs that are currenty dirty
    intrusive_list_t<buf_t> dirty_bufs;
    
    sync_callback_t *shutdown_callback;
    bool in_shutdown_sync;
    bool transaction_backdoor;

    /* Internal variables used only during a flush operation. */
    
    enum state state;
    // Transaction that the writeback is using to grab buffers
    transaction_t *transaction;
    // List of buffers being written during the current writeback
    intrusive_list_t<buf_t> flush_bufs;
    // List of things to call back as soon as the writeback currently in progress is over.
    intrusive_list_t<sync_callback_t> current_sync_callbacks;
};

#include "buffer_cache/writeback/writeback.tcc"

#endif // __buffer_cache_writeback_hpp__

