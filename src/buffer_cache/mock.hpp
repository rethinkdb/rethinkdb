#ifndef __BUFFER_CACHE_MOCK_HPP__
#define __BUFFER_CACHE_MOCK_HPP__

#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "containers/segmented_vector.hpp"
#include "utils.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "config/cmd_args.hpp"
#include "concurrency/rwi_lock.hpp"

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
    virtual ~mock_transaction_begin_callback_t() {}
};

struct mock_transaction_commit_callback_t {
    virtual void on_txn_commit(mock_transaction_t *txn) = 0;
    virtual ~mock_transaction_commit_callback_t() {}
};

struct mock_block_available_callback_t {
    virtual void on_block_available(mock_buf_t *buf) = 0;
    virtual ~mock_block_available_callback_t() {}
};

/* Buf */

class mock_buf_t :
    public lock_available_callback_t
{
    typedef mock_block_available_callback_t block_available_callback_t;
    
public:
    block_id_t get_block_id();
    void *get_data_write();
    const void *get_data_read();
    void mark_deleted();
    void release();
    bool is_dirty();

private:
    void on_lock_available();
    friend class mock_transaction_t;
    friend class mock_cache_t;
    friend class internal_buf_t;
    internal_buf_t *internal_buf;
    access_t access;
    bool dirty, deleted;
    block_available_callback_t *cb;
    mock_buf_t(internal_buf_t *buf, access_t access);
};

/* Transaction */

class mock_transaction_t
{
    typedef mock_buf_t buf_t;
    typedef mock_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mock_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mock_block_available_callback_t block_available_callback_t;

public:
    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode, block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);
    repli_timestamp get_subtree_recency(block_id_t block_id);

    mock_cache_t *cache;

private:
    friend class mock_cache_t;
    access_t access;
    int n_bufs;
    void finish_committing(mock_transaction_commit_callback_t *cb);
    mock_transaction_t(mock_cache_t *cache, access_t access);
    ~mock_transaction_t();
};

/* Cache */

class mock_cache_t :
    public home_cpu_mixin_t,
    public serializer_t::read_callback_t,
    public serializer_t::write_txn_callback_t
{
public:
    typedef mock_buf_t buf_t;
    typedef mock_transaction_t transaction_t;
    typedef mock_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mock_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mock_block_available_callback_t block_available_callback_t;
    
    mock_cache_t(
        // mock_cache gets a serializer so its constructor is consistent with
        // the mirrored cache's serializer, but it doesn't use it.
        translator_serializer_t *serializer,
        mirrored_cache_config_t *config);
    ~mock_cache_t();
    
    struct ready_callback_t {
        virtual void on_cache_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start(ready_callback_t *cb);
    
    block_size_t get_block_size();
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback);
    
    struct shutdown_callback_t {
        virtual void on_cache_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    bool shutdown(shutdown_callback_t *cb);

private:
    friend class mock_transaction_t;
    friend class mock_buf_t;
    friend class internal_buf_t;
    
    translator_serializer_t *serializer;
    bool running;
    int n_transactions;
    block_size_t block_size;
    segmented_vector_t<internal_buf_t *, MAX_BLOCK_ID> bufs;
    
    ready_callback_t *ready_callback;
    bool load_blocks_from_serializer();
    int blocks_to_load;
    void on_serializer_read();
    bool have_loaded_blocks();
    
    shutdown_callback_t *shutdown_callback;
    bool shutdown_write_bufs();
    bool shutdown_do_send_bufs_to_serializer();
    void on_serializer_write_txn();
    bool shutdown_destroy_bufs();
    void shutdown_finish();
};

#endif /* __BUFFER_CACHE_MOCK_HPP__ */
