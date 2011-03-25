#ifndef __BUFFER_CACHE_MOCK_HPP__
#define __BUFFER_CACHE_MOCK_HPP__

#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "containers/segmented_vector.hpp"
#include "utils.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "server/cmd_args.hpp"
#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/buf_patch.hpp"

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
    const void *get_data_read();
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    void *get_data_major_write();
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    void set_data(void *dest, const void *src, const size_t n);
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    void move_data(void *dest, const void *src, const size_t n);
    void apply_patch(buf_patch_t *patch); // This might delete the supplied patch, do not use patch after its application
    patch_counter_t get_next_patch_counter();
    void mark_deleted(bool write_null = true);
    void touch_recency(repli_timestamp timestamp);
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

    void snapshot() { }

    buf_t *acquire(block_id_t block_id, access_t mode, block_available_callback_t *callback, bool should_load = true);
    buf_t *allocate();
    void get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out);

    mock_cache_t *cache;

private:
    friend class mock_cache_t;
    access_t access;
    int n_bufs;
    repli_timestamp recency_timestamp;
    void finish_committing(mock_transaction_commit_callback_t *cb);
    mock_transaction_t(mock_cache_t *cache, access_t access, repli_timestamp recency_timestamp);
    ~mock_transaction_t();
};

/* Cache */

class mock_cache_t :
    public home_thread_mixin_t
{
public:
    typedef mock_buf_t buf_t;
    typedef mock_transaction_t transaction_t;
    typedef mock_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mock_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mock_block_available_callback_t block_available_callback_t;

    static void create(
        translator_serializer_t *serializer,
        mirrored_cache_static_config_t *static_config);
    mock_cache_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config);
    ~mock_cache_t();

    block_size_t get_block_size();
    transaction_t *begin_transaction(access_t access, int expected_change_count, repli_timestamp recency_timestamp, transaction_begin_callback_t *callback);

private:
    friend class mock_transaction_t;
    friend class mock_buf_t;
    friend class internal_buf_t;

    translator_serializer_t *serializer;
    drain_semaphore_t transaction_counter;
    block_size_t block_size;
    segmented_vector_t<internal_buf_t *, MAX_BLOCK_ID> bufs;
};

#endif /* __BUFFER_CACHE_MOCK_HPP__ */
