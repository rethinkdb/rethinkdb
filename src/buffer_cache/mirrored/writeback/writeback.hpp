
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include "concurrency/rwi_lock.hpp"
#include "flush_time_randomizer.hpp"
#include "utils.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "buffer_cache/mirrored/callbacks.hpp"

struct mc_cache_t;
struct mc_buf_t;
struct mc_inner_buf_t;
struct mc_transaction_t;

struct writeback_t :
    public lock_available_callback_t,
    public serializer_t::write_txn_callback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_transaction_t transaction_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;
    
public:
    writeback_t(
        cache_t *cache,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold,
        unsigned int max_dirty_blocks
        );
    virtual ~writeback_t();
    
    struct sync_callback_t : public intrusive_list_node_t<sync_callback_t> {
        virtual ~sync_callback_t() {}
        virtual void on_sync() = 0;
    };
    
    /* Forces a writeback to happen soon. If there is nothing to write, return 'true'; otherwise,
    returns 'false' and calls 'callback' as soon as the next writeback cycle is over. */
    bool sync(sync_callback_t *callback);
    
    /* Same as sync(), but doesn't hurry up the writeback in any way. */
    bool sync_patiently(sync_callback_t *callback);

    bool begin_transaction(transaction_t *txn, transaction_begin_callback_t *cb);
    void on_transaction_commit(transaction_t *txn);

    struct begin_transaction_fsm_t :
        public thread_message_t,
        public lock_available_callback_t,
        public intrusive_list_node_t<begin_transaction_fsm_t>
    {
        writeback_t *writeback;
        mc_transaction_t *transaction;
        mc_transaction_begin_callback_t *callback;
        begin_transaction_fsm_t(writeback_t *wb, mc_transaction_t *txn, mc_transaction_begin_callback_t *cb)
            : writeback(wb), transaction(txn), callback(cb)
        {
            if (writeback->too_many_dirty_blocks()) {
                /* When a flush happens, we will get popped off the throttled transactions list
                and given a green light. */
                writeback->throttled_transactions_list.push_back(this);
            } else {
                green_light();
            }
        }
        void green_light() {
            // Lock the flush lock "for reading", but what we really mean is to lock it non-
            // exclusively because more than one write transaction can proceed at once.
            if (writeback->flush_lock.lock(rwi_read, this)) {
                /* push callback on the global event queue - if the lock
                 * returns true, then the request won't have been put on the
                 * flush lock's queue, which can lead to the
                 * transaction_begin_callback not being called when it should be
                 * in certain cases, as shown by append_stress append
                 * reorderings.
                 */
                call_later_on_this_thread(this);
            }
        }
        void on_thread_switch() {
            on_lock_available();
        }
        void on_lock_available() {
            callback->on_txn_begin(transaction);
            delete this;
        }
    };
    intrusive_list_t<begin_transaction_fsm_t> throttled_transactions_list;

    unsigned int num_dirty_blocks();

    bool too_many_dirty_blocks();
    void possibly_unthrottle_transactions();

    class local_buf_t : public intrusive_list_node_t<local_buf_t> {
        friend class writeback_t;
        
    public:
        explicit local_buf_t(inner_buf_t *gbuf)
            : gbuf(gbuf), dirty(false), recency_dirty(false) {}
        
        void set_dirty(bool _dirty = true);
        void set_recency_dirty(bool _recency_dirty = true);
        
        bool safe_to_unload() const { return !dirty && !recency_dirty; }

    private:
        inner_buf_t *gbuf;
    public: //TODO make this private again @jdoliner
        bool dirty;
        bool recency_dirty;
    };
    
    /* User-controlled settings. */
    
    bool wait_for_flush;
    
    unsigned int max_dirty_blocks;   // Number of blocks, not percentage
    
private:
    flush_time_randomizer_t flush_time_randomizer;
    unsigned int flush_threshold;   // Number of blocks, not percentage
    
private:    
    // The writeback system has a mechanism to keep data safe if the server crashes. If modified
    // data sits in memory for longer than flush_timer_ms milliseconds, a writeback will be
    // automatically started to store it on disk. flush_timer is the timer to keep track of how
    // much longer the data can sit in memory.
    timer_token_t *flush_timer;
    static void flush_timer_callback(void *ctx);
    
    bool writeback_in_progress;
    
    /* Functions and callbacks for different phases of the writeback */
    
    bool writeback_start_and_acquire_lock();   // Called on cache thread
    virtual void on_lock_available();   // Called on cache thread
    bool writeback_acquire_bufs();   // Called on cache thread
    bool writeback_do_write();   // Called on serializer thread
    void on_serializer_write_txn();   // Called on serializer thread
    bool writeback_do_cleanup();   // Called on cache thread
    
    cache_t *cache;

    /* The flush lock is necessary because if we acquire dirty blocks
     * in random order during the flush, there might be a deadlock
     * with set_fsm. When the flush is initiated, the flush_lock is
     * grabbed, all write transactions are drained, and no new write transactions
     * are allowed in the meantime. Once all transactions are drained,
     * the writeback code locks all dirty blocks for reading and
     * releases the flush lock. */
    
    /* Note that the write transactions lock the flush lock for reading, because there can be more
    than one write transaction proceeding at the same time (not on the same bufs, but the bufs'
    locks will take care of that) while the writeback locks the flush lock for writing, because it
    must exclude all of the transactions.
    */
    
    rwi_lock_t flush_lock;
    
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
    intrusive_list_t<local_buf_t> dirty_bufs;

    /* Internal variables used only during a flush operation. */
    
    ticks_t start_time;
    
    // Transaction that the writeback is using to grab buffers
    transaction_t *transaction;

    // Transaction to submit to the serializer
    std::vector<translator_serializer_t::write_t> serializer_writes;

    // List of things to call back as soon as the writeback currently in progress is over.
    intrusive_list_t<sync_callback_t> current_sync_callbacks;
};

#endif // __BUFFER_CACHE_WRITEBACK_HPP__

