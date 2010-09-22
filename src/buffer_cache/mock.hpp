#ifndef __BUFFER_CACHE_MOCK_HPP__
#define __BUFFER_CACHE_MOCK_HPP__

#include "arch/resource.hpp"
#include "concurrency/access.hpp"
#include "containers/segmented_vector.hpp"

/* The mock cache, mock_cache_t, is a drop-in replacement for mc_cache_t that keeps all of
its contents in memory and artificially generates delays in responding to requests. It
should not be used in production; its purpose is to help catch bugs in the btree code. */

class mock_buf_t;
class mock_transaction_t;
class mock_cache_t;
class internal_buf_t;

/* Callbacks */

struct mock_transaction_begin_callback_t {
    virtual void on_txn_begin(mock_transaction_t *txn) = 0;
};

struct mock_transaction_commit_callback_t {
    virtual void on_txn_commit(mock_transaction_t *txn) = 0;
};

struct mock_block_available_callback_t {
    virtual void on_block_available(mock_buf_t *buf) = 0;
};

/* Buf */

class mock_buf_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, mock_buf_t >
{
    typedef mock_block_available_callback_t block_available_callback_t;

public:
    block_id_t get_block_id();
    void *get_data();
    void set_dirty();
    void mark_deleted();
    void release();

private:
    friend class mock_transaction_t;
    friend class mock_cache_t;
    friend class internal_buf_t;
    internal_buf_t *internal_buf;
    access_t access;
    bool dirty, deleted;
    mock_buf_t(internal_buf_t *buf, access_t access);
};

/* Transaction */

class mock_transaction_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, mock_transaction_t >
{
    typedef mock_buf_t buf_t;
    typedef mock_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mock_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mock_block_available_callback_t block_available_callback_t;

public:
    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode, block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);

private:
    friend class mock_cache_t;
    mock_cache_t *cache;
    access_t access;
    mock_transaction_t(mock_cache_t *cache, access_t access);
};

/* Cache */

class mock_cache_t {
    
public:
    typedef mock_buf_t buf_t;
    typedef mock_transaction_t transaction_t;
    typedef mock_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mock_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mock_block_available_callback_t block_available_callback_t;
    
    mock_cache_t(
        char *filename,
        size_t _block_size,
        size_t _max_size,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold_percent);
    ~mock_cache_t();
    
    struct ready_callback_t {
        virtual void on_cache_ready() = 0;
    };
    bool start(ready_callback_t *cb);
    
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback);
    
    struct shutdown_callback_t {
        virtual void on_cache_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

private:
    friend class mock_transaction_t;
    friend class mock_buf_t;
    friend class internal_buf_t;
    buffer_alloc_t alloc;
    bool running;
    size_t block_size;
    segmented_vector_t<internal_buf_t *, MAX_BLOCK_ID> bufs;
};

#endif /* __BUFFER_CACHE_MOCK_HPP__ */
