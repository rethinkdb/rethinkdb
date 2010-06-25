
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include <set>

#include "concurrency/rwi_lock.hpp"

template <class config_t>
struct writeback_tmpl_t : public lock_available_callback<config_t> {
public:
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    
    writeback_tmpl_t(cache_t *cache, bool delay, unsigned int interval_ms);
    virtual ~writeback_tmpl_t();

    void start(); // Start the writeback thread as part of database startup.

    bool begin_transaction(transaction_t *txn);
    bool commit(transaction_t *transaction,
        transaction_commit_callback<config_t> *callback);
    void aio_complete(buf_t *, bool written);

    class local_buf_t {
    public:
        explicit local_buf_t(writeback_tmpl_t *wb)
            : writeback(wb), dirty(false) {}

        bool is_dirty() const { return dirty; }
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
    typedef typename serializer_t::block_id_t block_id_t;
    typedef transaction_commit_callback<config_t> transaction_commit_callback_t;
    typedef std::pair<transaction_t *, transaction_commit_callback_t *>
        txn_state_t;

    enum state {
        state_none,
        state_locking,
        state_locked,
        state_write_bufs,
    };

    static void timer_callback(void *ctx);
    virtual void on_lock_available();
    void writeback(buf_t *buf);

    /* User-controlled settings. */
    bool delay_commits;
    uint64_t interval_ms;

    /* Internal variables used at all times. */
    cache_t *cache;
    rwi_lock<config_t> *flush_lock;
    std::set<txn_state_t> txns;
    std::set<block_id_t> dirty_blocks;

    /* Internal variables used only during a flush operation. */
    enum state state;
    transaction_t *transaction;
    std::set<txn_state_t> flush_txns;
    std::set<buf_t *> flush_bufs;
};

#include "buffer_cache/writeback/writeback_impl.hpp"

#endif // __buffer_cache_writeback_hpp__

